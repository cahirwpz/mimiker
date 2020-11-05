#define KL_LOG KL_DEV
#include <sys/libkern.h>
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

#define NS16550_VENDOR_ID 0x8086
#define NS16550_DEVICE_ID 0x7110

#define UART_BUFSIZE 128

typedef struct ns16550_state {
  spin_t lock;
  condvar_t tx_nonfull, rx_nonempty;
  ringbuf_t tx_buf, rx_buf;
  intr_handler_t intr_handler;
  resource_t *regs;
} ns16550_state_t;

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

static int ns16550_read(vnode_t *v, uio_t *uio, int ioflags) {
  ns16550_state_t *ns16550 = v->v_data;
  int error;

  uio->uio_offset = 0; /* This device does not support offsets. */

  uint8_t byte;
  WITH_SPIN_LOCK (&ns16550->lock) {
    /* For simplicity, copy to the user space one byte at a time. */
    while (!ringbuf_getb(&ns16550->rx_buf, &byte))
      cv_wait(&ns16550->rx_nonempty, &ns16550->lock);
  }
  if ((error = uiomove_frombuf(&byte, 1, uio)))
    return error;

  return 0;
}

static int ns16550_write(vnode_t *v, uio_t *uio, int ioflags) {
  ns16550_state_t *ns16550 = v->v_data;
  resource_t *uart = ns16550->regs;
  int error;

  uio->uio_offset = 0; /* This device does not support offsets. */

  while (uio->uio_resid > 0) {
    uint8_t byte;
    /* For simplicity, copy from the user space one byte at a time. */
    if ((error = uiomove(&byte, 1, uio)))
      return error;
    WITH_SPIN_LOCK (&ns16550->lock) {
      if (in(uart, LSR) & LSR_THRE) {
        out(uart, THR, byte);
      } else {
        while (!ringbuf_putb(&ns16550->tx_buf, byte))
          cv_wait(&ns16550->tx_nonfull, &ns16550->lock);
      }
    }
  }

  return 0;
}

static int ns16550_close(vnode_t *v, file_t *fp) {
  /* TODO release resources */
  return 0;
}

/* XXX: This should be implemented by tty driver, not here. */
static int ns16550_ioctl(vnode_t *v, u_long cmd, void *data) {
  if (cmd) {
    unsigned len = IOCPARM_LEN(cmd);
    memset(data, 0, len);
    return 0;
  }

  return EPASSTHROUGH;
}

static int ns16550_getattr(vnode_t *v, vattr_t *va) {
  memset(va, 0, sizeof(vattr_t));
  va->va_mode = S_IFCHR;
  va->va_nlink = 1;
  va->va_ino = 0;
  va->va_size = 0;
  return 0;
}

static vnodeops_t dev_uart_ops = {.v_open = vnode_open_generic,
                                  .v_write = ns16550_write,
                                  .v_read = ns16550_read,
                                  .v_close = ns16550_close,
                                  .v_ioctl = ns16550_ioctl,
                                  .v_getattr = ns16550_getattr};

static intr_filter_t ns16550_intr(void *data) {
  ns16550_state_t *ns16550 = data;
  resource_t *uart = ns16550->regs;
  intr_filter_t res = IF_STRAY;

  WITH_SPIN_LOCK (&ns16550->lock) {
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

  ns16550_state_t *ns16550 = dev->state;

  ns16550->rx_buf.data = kmalloc(M_DEV, UART_BUFSIZE, M_ZERO);
  ns16550->rx_buf.size = UART_BUFSIZE;
  ns16550->tx_buf.data = kmalloc(M_DEV, UART_BUFSIZE, M_ZERO);
  ns16550->tx_buf.size = UART_BUFSIZE;

  spin_init(&ns16550->lock, 0);
  cv_init(&ns16550->rx_nonempty, "UART receive buffer not empty");
  cv_init(&ns16550->tx_nonfull, "UART transmit buffer not full");

  /* TODO Small hack to select COM1 UART */
  ns16550->regs = bus_alloc_resource(
    dev, RT_ISA, 0, IO_COM1, IO_COM1 + IO_COMSIZE - 1, IO_COMSIZE, RF_ACTIVE);
  assert(ns16550->regs != NULL);
  ns16550->intr_handler =
    INTR_HANDLER_INIT(ns16550_intr, NULL, ns16550, "NS16550 UART", 0);
  /* TODO Do not use magic number "4" here! */
  bus_intr_setup(dev, 4, &ns16550->intr_handler);

  /* Setup UART and enable interrupts */
  setup(ns16550->regs);
  out(ns16550->regs, IER, IER_ERXRDY | IER_ETXRDY);

  /* Prepare /dev/uart interface. */
  devfs_makedev(NULL, "uart", &dev_uart_ops, ns16550);

  return 0;
}

static int ns16550_probe(device_t *dev) {
  pci_device_t *pcid = pci_device_of(dev);
  if (pcid->vendor_id != NS16550_VENDOR_ID || pcid->device_id != NS16550_DEVICE_ID)
    return 0;
  return 1;
}

/* clang-format off */
static driver_t ns16550_driver = {
  .desc = "NS16550 UART driver",
  .size = sizeof(ns16550_state_t),
  .attach = ns16550_attach,
  .identify = bus_generic_identify,
  .probe = ns16550_probe,
};
/* clang-format on */

DEVCLASS_ENTRY(pci, ns16550_driver);
