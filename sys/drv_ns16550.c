#define KL_LOG KL_DEV
#include <stdc.h>
#include <vnode.h>
#include <devfs.h>
#include <klog.h>
#include <condvar.h>
#include <ringbuf.h>
#include <pci.h>
#include <dev/isareg.h>
#include <dev/ns16550reg.h>
#include <interrupt.h>
#include <sysinit.h>

#define UART_BUFSIZE 128

typedef struct ns16550_state {
  mtx_t mtx;
  condvar_t tx_nonfull, rx_nonempty;
  ringbuf_t tx_buf, rx_buf;
  intr_handler_t intr_handler;
  resource_t regs;
} ns16550_state_t;

static uint8_t in(resource_t *regs, unsigned offset) {
  return bus_space_read_1(regs, offset);
}

static void out(resource_t *regs, unsigned offset, uint8_t value) {
  bus_space_write_1(regs, offset, value);
}

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

static int uart_read(vnode_t *v, uio_t *uio) {
  ns16550_state_t *ns16550 = v->v_data;
  int error;

  uio->uio_offset = 0; /* This device does not support offsets. */

  WITH_MTX_LOCK (&ns16550->mtx) {
    uint8_t byte;
    /* For simplicity, copy to the user space one byte at a time. */
    while (!ringbuf_getb(&ns16550->rx_buf, &byte))
      cv_wait(&ns16550->rx_nonempty, &ns16550->mtx);
    if ((error = uiomove_frombuf(&byte, 1, uio)))
      return error;
  }

  return 0;
}

static int uart_write(vnode_t *v, uio_t *uio) {
  ns16550_state_t *ns16550 = v->v_data;
  int error;

  uio->uio_offset = 0; /* This device does not support offsets. */

  WITH_MTX_LOCK (&ns16550->mtx) {
    while (uio->uio_resid > 0) {
      uint8_t byte;
      /* For simplicity, copy from the user space one byte at a time. */
      if ((error = uiomove(&byte, 1, uio)))
        return error;
      while (!ringbuf_putb(&ns16550->rx_buf, byte))
        cv_wait(&ns16550->tx_nonfull, &ns16550->mtx);
    }
  }

  return 0;
}

static vnodeops_t dev_uart_ops = {
  .v_lookup = vnode_lookup_nop,
  .v_readdir = vnode_readdir_nop,
  .v_open = vnode_open_generic,
  .v_write = uart_write,
  .v_read = uart_read,
};

static intr_filter_t ns16550_intr(void *data) {
  ns16550_state_t *ns16550 = data;
  resource_t *uart = &ns16550->regs;
  intr_filter_t res = IF_STRAY;

  WITH_MTX_LOCK (&ns16550->mtx) {
    uint8_t iir = in(uart, IIR);

    /* data ready to be received? */
    if (iir & IIR_RXRDY) {
      (void)ringbuf_putb(&ns16550->rx_buf, in(uart, RBR));
      cv_signal(&ns16550->rx_nonempty);
      res = IF_FILTERED;
    }

    /* transmit register empty? */
    if (iir & IIR_TXRDY) {
      uint8_t byte;
      if (ringbuf_getb(&ns16550->tx_buf, &byte)) {
        out(uart, THR, byte);
        cv_signal(&ns16550->tx_nonfull);
      }
      res = IF_FILTERED;
    }
  }

  return res;
}

static int ns16550_attach(device_t *dev) {
  assert(dev->parent->bus == DEV_BUS_PCI);

  pci_bus_state_t *pcib = dev->parent->state;
  ns16550_state_t *ns16550 = dev->state;

  ns16550->rx_buf.data = kmalloc(M_DEV, UART_BUFSIZE, M_ZERO);
  ns16550->rx_buf.size = UART_BUFSIZE;
  ns16550->tx_buf.data = kmalloc(M_DEV, UART_BUFSIZE, M_ZERO);
  ns16550->tx_buf.size = UART_BUFSIZE;

  mtx_init(&ns16550->mtx, MTX_DEF);
  cv_init(&ns16550->rx_nonempty, "UART receive buffer not empty");
  cv_init(&ns16550->tx_nonfull, "UART transmit buffer not full");

  /* TODO Nasty hack to select COM1 UART */
  ns16550->regs = *pcib->io_space;
  ns16550->regs.r_start += IO_COM1;

  ns16550->intr_handler =
    INTR_HANDLER_INIT(ns16550_intr, NULL, ns16550, "NS16550 UART", 0);
  bus_intr_setup(dev, 4, &ns16550->intr_handler);

  /* Setup UART and enable interrupts */
  setup(&ns16550->regs);
  out(&ns16550->regs, IER, IER_ERXRDY | IER_ETXRDY);

  /* Prepare /dev/uart interface. */
  vnode_t *uart_device = vnode_new(V_DEV, &dev_uart_ops);
  uart_device->v_data = ns16550;
  devfs_install("uart", uart_device);

  return 0;
}

static driver_t ns16550_driver = {
  .desc = "NS16550 UART driver",
  .size = sizeof(ns16550_state_t),
  .attach = ns16550_attach,
};

extern device_t *gt_pci;

static void ns16550_init(void) {
  (void)make_device(gt_pci, &ns16550_driver);
}

SYSINIT_ADD(ns16550, ns16550_init, DEPS("rootdev"));
