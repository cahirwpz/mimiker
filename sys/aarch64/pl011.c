#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/bus.h>
#include <sys/devclass.h>
#include <sys/rman.h>
#include <sys/vnode.h>
#include <sys/devfs.h>
#include <sys/stat.h>
#include <sys/libkern.h>
#include <sys/ttycom.h>
#include <sys/interrupt.h>
#include <sys/ringbuf.h>
#include <sys/tty.h>
#include <sys/sched.h>
#include <aarch64/bcm2835reg.h>
#include <aarch64/bcm2835_gpioreg.h>
#include <aarch64/plcomreg.h>
#include <aarch64/gpio.h>

#define UART0_BASE BCM2835_PERIPHERALS_BUS_TO_PHYS(BCM2835_UART0_BASE)
#define UART_BUFSIZE 128

static inline void set4(resource_t *r, int o, uint32_t v) {
  bus_write_4(r, o, bus_read_4(r, o) | v);
}

static inline void clr4(resource_t *r, int o, uint32_t v) {
  set4(r, o, bus_read_4(r, o) & ~v);
}

typedef struct pl011_state {
  spin_t lock;
  ringbuf_t rx_buf;
  ringbuf_t tx_buf;
  resource_t *regs;
  resource_t *irq;
  tty_t *tty;
  condvar_t tty_thread_cv;
  uint8_t tty_thread_flags;
} pl011_state_t;

/* tty->t_outq -> tx_buf */
#define TTY_THREAD_TXRDY 0x1
/* rx_buf -> tty->t_inq */
#define TTY_THREAD_RXRDY 0x2
#define TTY_THREAD_WORK_MASK (TTY_THREAD_TXRDY | TTY_THREAD_RXRDY)
/* If cleared, don't wake up worker thread from pl011_intr. */
#define TTY_THREAD_OUTQ_NONEMPTY 0x4

/* Return true if we can write. */
static bool pl011_wready(pl011_state_t *state) {
  resource_t *r = state->regs;
  uint32_t reg = bus_read_4(r, PL01XCOM_FR);
  return (reg & PL01X_FR_TXFF) == 0;
}

static void pl011_putc(pl011_state_t *state, uint32_t c) {
  assert(pl011_wready(state));

  resource_t *r = state->regs;
  bus_write_4(r, PL01XCOM_DR, c);
}

/* Return true if we can read. */
static bool pl011_rready(pl011_state_t *state) {
  resource_t *r = state->regs;
  uint32_t reg = bus_read_4(r, PL01XCOM_FR);
  return (reg & PL01X_FR_RXFE) == 0;
}

static uint32_t pl011_getc(pl011_state_t *state) {
  assert(pl011_rready(state));

  resource_t *r = state->regs;
  return bus_read_4(r, PL01XCOM_DR);
}

static intr_filter_t pl011_intr(void *data /* device_t* */) {
  pl011_state_t *pl011 = ((device_t *)data)->state;
  intr_filter_t res = IF_STRAY;

  WITH_SPIN_LOCK (&pl011->lock) {
    if (pl011_rready(pl011)) {
      ringbuf_putb(&pl011->rx_buf, pl011_getc(pl011));
      pl011->tty_thread_flags |= TTY_THREAD_RXRDY;
      cv_signal(&pl011->tty_thread_cv);
      res = IF_FILTERED;
    }

    if (pl011_wready(pl011)) {
      uint8_t byte;
      while (pl011_wready(pl011) && ringbuf_getb(&pl011->tx_buf, &byte)) {
        pl011_putc(pl011, byte);
      }
      if (ringbuf_empty(&pl011->tx_buf)) {
        /* If we're out of characters and there are characters
         * in the tty's output queue, signal the tty thread to refill. */
        if (pl011->tty_thread_flags & TTY_THREAD_OUTQ_NONEMPTY) {
          pl011->tty_thread_flags |= TTY_THREAD_TXRDY;
          cv_signal(&pl011->tty_thread_cv);
        }
        /* Disable TXRDY interrupts - the tty thread will re-enable them
         * after filling tx_buf. */
        clr4(pl011->regs, PL011COM_CR, PL011_CR_TXE);
      }
      res = IF_FILTERED;
    }
  }

  return res;
}

/*
 * If tx_buf is empty, we can try to write characters directly from tty->t_outq.
 * This routine attempts to do just that.
 * Must be called with both tty->t_lock and pl011->lock held.
 */
static void pl011_try_bypass_txbuf(pl011_state_t *pl011, tty_t *tty) {
  uint8_t byte;

  if (!ringbuf_empty(&pl011->tx_buf))
    return;

  while (pl011_wready(pl011) && ringbuf_getb(&tty->t_outq, &byte)) {
    pl011_putc(pl011, byte);
  }
}

static void pl011_set_tty_outq_nonempty_flag(pl011_state_t *pl011, tty_t *tty) {
  if (ringbuf_empty(&tty->t_outq))
    pl011->tty_thread_flags &= ~TTY_THREAD_OUTQ_NONEMPTY;
  else
    pl011->tty_thread_flags |= TTY_THREAD_OUTQ_NONEMPTY;
}

/*
 * Move characters from tty->t_outq to pl011->tx_buf.
 * Must by called with tty->t_lock held.
 */
static void pl011_fill_txbuf(pl011_state_t *pl011, tty_t *tty) {
  uint8_t byte;

  while (true) {
    SCOPED_SPIN_LOCK(&pl011->lock);
    pl011_try_bypass_txbuf(pl011, tty);
    if (ringbuf_full(&pl011->tx_buf) || !ringbuf_getb(&tty->t_outq, &byte)) {
      pl011_set_tty_outq_nonempty_flag(pl011, tty);
      break;
    }
    if (!ringbuf_empty(&pl011->tx_buf))
      set4(pl011->regs, PL011COM_CR, PL011_CR_TXE);
    pl011_set_tty_outq_nonempty_flag(pl011, tty);
    ringbuf_putb(&pl011->tx_buf, byte);
  }
  tty_getc_done(tty);
}

static bool pl011_getb_lock(pl011_state_t *pl011, uint8_t *byte_p) {
  SCOPED_SPIN_LOCK(&pl011->lock);
  return ringbuf_getb(&pl011->rx_buf, byte_p);
}

/* TODO: revisit after per-intr_event ithreads are implemented. */
static void pl011_tty_thread(void *arg) {
  pl011_state_t *pl011 = (pl011_state_t *)arg;
  tty_t *tty = pl011->tty;
  uint8_t work, byte;

  while (true) {
    WITH_SPIN_LOCK (&pl011->lock) {
      /* Sleep until there's work for us to do. */
      while ((work = pl011->tty_thread_flags & TTY_THREAD_WORK_MASK) == 0)
        cv_wait(&pl011->tty_thread_cv, &pl011->lock);
      pl011->tty_thread_flags &= ~TTY_THREAD_WORK_MASK;
    }
    WITH_MTX_LOCK (&tty->t_lock) {
      if (work & TTY_THREAD_RXRDY) {
        /* Move characters from rx_buf into the tty's input queue. */
        while (pl011_getb_lock(pl011, &byte))
          if (!tty_input(tty, byte))
            klog("dropped character %hhx", byte);
      }
      if (work & TTY_THREAD_TXRDY) {
        pl011_fill_txbuf(pl011, tty);
      }
    }
  }
}

/*
 * New characters have appeared in the tty's output queue.
 * Fill the UART's tx_buf and enable TXRDY interrupts.
 * Called with `tty->t_lock` held.
 */
static void pl011_notify_out(tty_t *tty) {
  pl011_state_t *pl011 = tty->t_data;

  if (ringbuf_empty(&tty->t_outq))
    return;

  pl011_fill_txbuf(pl011, tty);
}

static int pl011_probe(device_t *dev) {
  /* (pj) so far we don't have better way to associate driver with device for
   * buses which do not automatically enumerate their children. */
  return (dev->unit == 1);
}

static int pl011_attach(device_t *dev) {
  pl011_state_t *state = dev->state;

  ringbuf_init(&state->rx_buf, kmalloc(M_DEV, UART_BUFSIZE, M_ZERO),
               UART_BUFSIZE);
  ringbuf_init(&state->tx_buf, kmalloc(M_DEV, UART_BUFSIZE, M_ZERO),
               UART_BUFSIZE);

  spin_init(&state->lock, 0);

  tty_t *tty = tty_alloc();
  tty->t_termios.c_ispeed = 115200;
  tty->t_termios.c_ospeed = 115200;
  tty->t_ops.t_notify_out = pl011_notify_out;
  tty->t_data = state;
  state->tty = tty;

  cv_init(&state->tty_thread_cv, "PL011 TTY thread notification");
  thread_t *tty_thread = thread_create("pl011-tty-worker", pl011_tty_thread,
                                       state, prio_ithread(PRIO_ITHRD_QTY - 1));
  sched_add(tty_thread);

  resource_t *r = device_take_memory(dev, 0, 0);

  /* (pj) BCM2835_UART0_SIZE is much smaller than PAGESIZE */
  bus_space_map(r->r_bus_tag, resource_start(r), PAGESIZE, &r->r_bus_handle);

  assert(r != NULL);

  state->regs = r;

  /* Disable UART0. */
  bus_write_4(r, PL011COM_CR, 0);
  /* Clear pending interrupts. */
  bus_write_4(r, PL011COM_ICR, PL011_INT_ALLMASK);

  /* TODO(pj) do magic with mail buffer */

  bcm2835_gpio_function_select(r, 14, BCM2835_GPIO_ALT0);
  bcm2835_gpio_function_select(r, 15, BCM2835_GPIO_ALT0);
  bcm2835_gpio_set_pull(r, 14, BCM2838_GPIO_GPPUD_PULLOFF);
  bcm2835_gpio_set_pull(r, 15, BCM2838_GPIO_GPPUD_PULLOFF);

  /*
   * Set integer & fractional part of baud rate.
   * Divider = UART_CLOCK/(16 * Baud)
   * Fraction part register = (Fractional part * 64) + 0.5
   * UART_CLOCK = 3000000; Baud = 115200.
   *
   * Divider = 4000000/(16 * 115200) = 2.17 = ~2.
   * Fractional part register = (.17 * 64) + 0.5 = 11.3 = ~11 = 0xb.
   */

  bus_write_4(r, PL011COM_IBRD, 2);
  bus_write_4(r, PL011COM_FBRD, 0xb);

  /* Enable FIFO & 8 bit data transmission (1 stop bit, no parity). */
  bus_write_4(r, PL011COM_LCRH, PL01X_LCR_FEN | PL01X_LCR_8BITS);

  /* Mask all interrupts. */
  bus_write_4(r, PL011COM_IMSC, PL011_INT_ALLMASK);

  /* Enable UART0, receive & transfer part of UART. */
  bus_write_4(r, PL011COM_CR, PL01X_CR_UARTEN | PL011_CR_TXE | PL011_CR_RXE);

  /* Enable interrupt. */
  bus_write_4(r, PL011COM_IMSC, PL011_INT_RX);

  state->irq = device_take_irq(dev, 0, RF_ACTIVE);
  bus_intr_setup(dev, state->irq, pl011_intr, NULL, dev, "PL011 UART");

  /* Prepare /dev/uart interface. */
  tty_makedev(NULL, "uart", tty);

  return 0;
}

static driver_t pl011_driver = {
  .desc = "PL011 UART driver",
  .size = sizeof(pl011_state_t),
  .pass = SECOND_PASS,
  .attach = pl011_attach,
  .probe = pl011_probe,
};

DEVCLASS_ENTRY(root, pl011_driver);
