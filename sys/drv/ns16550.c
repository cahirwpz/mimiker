#define KL_LOG KL_DEV
#include <sys/libkern.h>
#include <sys/spinlock.h>
#include <sys/vnode.h>
#include <sys/devfs.h>
#include <sys/klog.h>
#include <sys/condvar.h>
#include <sys/ringbuf.h>
#include <sys/pci.h>
#include <sys/termios.h>
#include <sys/ttycom.h>
#include <dev/isareg.h>
#include <dev/ns16550reg.h>
#include <sys/interrupt.h>
#include <sys/stat.h>
#include <sys/devclass.h>
#include <sys/tty.h>
#include <sys/priority.h>
#include <sys/sched.h>

#define NS16550_VENDOR_ID 0x8086
#define NS16550_DEVICE_ID 0x7110

#define UART_BUFSIZE 128

typedef struct ns16550_state {
  spin_t lock;
  ringbuf_t tx_buf, rx_buf;
  resource_t *irq_res;
  resource_t *regs;
  tty_t *tty;
  condvar_t tty_thread_cv;
  uint8_t tty_thread_flags;
} ns16550_state_t;

/* tty->t_outq -> tx_buf */
#define TTY_THREAD_TXRDY 0x1
/* rx_buf -> tty->t_inq */
#define TTY_THREAD_RXRDY 0x2
#define TTY_THREAD_WORK_MASK (TTY_THREAD_TXRDY | TTY_THREAD_RXRDY)
/* If cleared, don't wake up worker thread from ns16550_intr. */
#define TTY_THREAD_OUTQ_NONEMPTY 0x4

#define in(regs, offset) bus_read_1((regs), (offset))
#define out(regs, offset, value) bus_write_1((regs), (offset), (value))

static void set(resource_t *regs, unsigned offset, uint8_t mask) {
  out(regs, offset, in(regs, offset) | mask);
}

static void clr(resource_t *regs, unsigned offset, uint8_t mask) {
  out(regs, offset, in(regs, offset) & ~mask);
}

static void setup(resource_t *regs) {
  set(regs, LCR, LCR_DLAB);
  out(regs, DLM, 0);
  out(regs, DLL, 1); /* 115200 */
  clr(regs, LCR, LCR_DLAB);

  out(regs, IER, 0);
  out(regs, FCR, 0);
  out(regs, LCR, LCR_8BITS); /* 8-bit data, no parity */
}

static intr_filter_t ns16550_intr(void *data) {
  ns16550_state_t *ns16550 = data;
  resource_t *uart = ns16550->regs;
  intr_filter_t res = IF_STRAY;

  WITH_SPIN_LOCK (&ns16550->lock) {
    uint8_t iir = in(uart, IIR);

    /* data ready to be received? */
    if (iir & IIR_RXRDY) {
      (void)ringbuf_putb(&ns16550->rx_buf, in(uart, RBR));
      ns16550->tty_thread_flags |= TTY_THREAD_RXRDY;
      cv_signal(&ns16550->tty_thread_cv);
      res = IF_FILTERED;
    }

    /* transmit register empty? */
    if (iir & IIR_TXRDY) {
      uint8_t byte;
      while ((in(uart, LSR) & LSR_THRE) &&
             ringbuf_getb(&ns16550->tx_buf, &byte)) {
        out(uart, THR, byte);
      }
      if (ringbuf_empty(&ns16550->tx_buf)) {
        /* If we're out of characters and there are characters
         * in the tty's output queue, signal the tty thread to refill. */
        if (ns16550->tty_thread_flags & TTY_THREAD_OUTQ_NONEMPTY) {
          ns16550->tty_thread_flags |= TTY_THREAD_TXRDY;
          cv_signal(&ns16550->tty_thread_cv);
        }
        /* Disable TXRDY interrupts - the tty thread will re-enable them
         * after filling tx_buf. */
        clr(uart, IER, IER_ETXRDY);
      }
      res = IF_FILTERED;
    }
  }

  return res;
}

/*
 * If tx_buf is empty, we can try to write characters directly from tty->t_outq.
 * This routine attempts to do just that.
 * Must be called with both tty->t_lock and ns16550->lock held.
 */
static void ns16550_try_bypass_txbuf(ns16550_state_t *ns16550, tty_t *tty) {
  resource_t *uart = ns16550->regs;
  uint8_t byte;

  if (!ringbuf_empty(&ns16550->tx_buf))
    return;

  while ((in(uart, LSR) & LSR_THRE) && ringbuf_getb(&tty->t_outq, &byte)) {
    out(uart, THR, byte);
  }
}

static void ns16550_set_tty_outq_nonempty_flag(ns16550_state_t *ns16550,
                                               tty_t *tty) {
  if (ringbuf_empty(&tty->t_outq))
    ns16550->tty_thread_flags &= ~TTY_THREAD_OUTQ_NONEMPTY;
  else
    ns16550->tty_thread_flags |= TTY_THREAD_OUTQ_NONEMPTY;
}

/*
 * Move characters from tty->t_outq to ns16550->tx_buf.
 * Must be called with tty->t_lock held.
 */
static void ns16550_fill_txbuf(ns16550_state_t *ns16550, tty_t *tty) {
  uint8_t byte;

  while (true) {
    SCOPED_SPIN_LOCK(&ns16550->lock);
    ns16550_try_bypass_txbuf(ns16550, tty);
    if (ringbuf_full(&ns16550->tx_buf) || !ringbuf_getb(&tty->t_outq, &byte)) {
      /* Enable TXRDY interrupts if there are characters in tx_buf. */
      if (!ringbuf_empty(&ns16550->tx_buf))
        set(ns16550->regs, IER, IER_ETXRDY);
      ns16550_set_tty_outq_nonempty_flag(ns16550, tty);
      break;
    }
    ns16550_set_tty_outq_nonempty_flag(ns16550, tty);
    ringbuf_putb(&ns16550->tx_buf, byte);
  }
}

static bool ns16550_getb_lock(ns16550_state_t *ns16550, uint8_t *byte_p) {
  SCOPED_SPIN_LOCK(&ns16550->lock);
  return ringbuf_getb(&ns16550->rx_buf, byte_p);
}

/* TODO: revisit after per-intr_event ithreads are implemented. */
static void ns16550_tty_thread(void *arg) {
  ns16550_state_t *ns16550 = (ns16550_state_t *)arg;
  tty_t *tty = ns16550->tty;
  uint8_t work, byte;

  while (true) {
    WITH_SPIN_LOCK (&ns16550->lock) {
      /* Sleep until there's work for us to do. */
      while ((work = ns16550->tty_thread_flags & TTY_THREAD_WORK_MASK) == 0)
        cv_wait(&ns16550->tty_thread_cv, &ns16550->lock);
      ns16550->tty_thread_flags &= ~TTY_THREAD_WORK_MASK;
    }
    WITH_MTX_LOCK (&tty->t_lock) {
      if (work & TTY_THREAD_RXRDY) {
        /* Move characters from rx_buf into the tty's input queue. */
        while (ns16550_getb_lock(ns16550, &byte))
          tty_input(tty, byte);
      }
      if (work & TTY_THREAD_TXRDY) {
        ns16550_fill_txbuf(ns16550, tty);
      }
    }
  }
}

/*
 * New characters have appeared in the tty's output queue.
 * Fill the UART's tx_buf and enable TXRDY interrupts.
 * Called with `tty->t_lock` held.
 */
static void ns16550_notify_out(tty_t *tty) {
  ns16550_state_t *ns16550 = tty->t_data;

  if (ringbuf_empty(&tty->t_outq))
    return;

  ns16550_fill_txbuf(ns16550, tty);
}

static int ns16550_attach(device_t *dev) {
  assert(dev->parent->bus == DEV_BUS_PCI);

  ns16550_state_t *ns16550 = dev->state;

  ringbuf_init(&ns16550->rx_buf, kmalloc(M_DEV, UART_BUFSIZE, M_ZERO),
               UART_BUFSIZE);
  ringbuf_init(&ns16550->tx_buf, kmalloc(M_DEV, UART_BUFSIZE, M_ZERO),
               UART_BUFSIZE);

  spin_init(&ns16550->lock, 0);

  tty_t *tty = tty_alloc();
  tty->t_termios.c_ispeed = 115200;
  tty->t_termios.c_ospeed = 115200;
  tty->t_ops.t_notify_out = ns16550_notify_out;
  tty->t_data = ns16550;
  ns16550->tty = tty;

  cv_init(&ns16550->tty_thread_cv, "NS16550 TTY thread notification");
  thread_t *tty_thread =
    thread_create("ns16550-tty-worker", ns16550_tty_thread, ns16550,
                  prio_ithread(PRIO_ITHRD_QTY - 1));
  sched_add(tty_thread);

  /* TODO Small hack to select COM1 UART */
  ns16550->regs =
    bus_alloc_resource(dev, RT_IOPORTS, 0, IO_COM1, IO_COM1 + IO_COMSIZE - 1,
                       IO_COMSIZE, RF_ACTIVE);
  assert(ns16550->regs != NULL);
  /* intr_handler name: "NS16550 UART" */
  ns16550->irq_res = bus_alloc_irq(dev, 0, 4 /* magic */, RF_ACTIVE);
  bus_intr_setup(dev, ns16550->irq_res, ns16550_intr, NULL, ns16550);

  /* Setup UART and enable interrupts */
  setup(ns16550->regs);
  out(ns16550->regs, IER, IER_ERXRDY | IER_ETXRDY);

  /* Prepare /dev/uart interface. */
  devfs_makedev(NULL, "uart", &tty_vnodeops, ns16550->tty);

  return 0;
}

static int ns16550_probe(device_t *dev) {
  pci_device_t *pcid = pci_device_of(dev);
  return pci_device_match(pcid, NS16550_VENDOR_ID, NS16550_DEVICE_ID);
}

/* clang-format off */
static driver_t ns16550_driver = {
  .desc = "NS16550 UART driver",
  .size = sizeof(ns16550_state_t),
  .attach = ns16550_attach,
  .probe = ns16550_probe,
};
/* clang-format on */

DEVCLASS_ENTRY(pci, ns16550_driver);
