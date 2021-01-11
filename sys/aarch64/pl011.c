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
#include <aarch64/bcm2835reg.h>
#include <aarch64/bcm2835_gpioreg.h>
#include <aarch64/plcomreg.h>
#include <aarch64/gpio.h>

#define UART0_BASE BCM2835_PERIPHERALS_BUS_TO_PHYS(BCM2835_UART0_BASE)
#define UART_BUFSIZE 128

typedef struct pl011_state {
  spin_t lock;
  condvar_t rx_nonempty;
  ringbuf_t rx_buf;
  condvar_t tx_nonfull;
  ringbuf_t tx_buf;
  resource_t *regs;
  resource_t *irq;
} pl011_state_t;

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

static int pl011_read(vnode_t *v, uio_t *uio, int ioflags) {
  pl011_state_t *state = devfs_node_data(v);

  /* This device does not support offsets. */
  uio->uio_offset = 0;

  uint8_t byte;
  WITH_SPIN_LOCK (&state->lock) {
    /* For simplicity, copy to the user space one byte at a time. */
    while (!ringbuf_getb(&state->rx_buf, &byte))
      cv_wait(&state->rx_nonempty, &state->lock);
  }

  return uiomove_frombuf(&byte, 1, uio);
}

static int pl011_write(vnode_t *v, uio_t *uio, int ioflags) {
  pl011_state_t *state = devfs_node_data(v);

  /* This device does not support offsets. */
  uio->uio_offset = 0;

  while (uio->uio_resid > 0) {
    uint8_t byte;
    int error;
    /* For simplicity, copy from the user space one byte at a time. */
    if ((error = uiomove(&byte, 1, uio)))
      return error;
    WITH_SPIN_LOCK (&state->lock) {
      if (pl011_wready(state)) {
        pl011_putc(state, byte);
      } else {
        while (!ringbuf_putb(&state->tx_buf, byte))
          cv_wait(&state->tx_nonfull, &state->lock);
      }
    }
  }

  return 0;
}

static int pl011_close(vnode_t *v, file_t *fp) {
  /* TODO release resources */
  return 0;
}

static int pl011_ioctl(vnode_t *v, u_long cmd, void *data) {
  if (cmd) {
    unsigned len = IOCPARM_LEN(cmd);
    memset(data, 0, len);
    return 0;
  }

  return EPASSTHROUGH;
}

static int pl011_getattr(vnode_t *v, vattr_t *va) {
  memset(va, 0, sizeof(vattr_t));
  va->va_mode = S_IFCHR;
  va->va_nlink = 1;
  va->va_ino = 0;
  va->va_size = 0;
  return 0;
}

/* clang-format off */
static vnodeops_t dev_uart_ops = {
  .v_write = pl011_write,
  .v_read = pl011_read,
  .v_close = pl011_close,
  .v_ioctl = pl011_ioctl,
  .v_getattr = pl011_getattr
};
/* clang-format on */

static intr_filter_t pl011_intr(void *data /* device_t* */) {
  pl011_state_t *state = ((device_t *)data)->state;
  intr_filter_t res = IF_STRAY;

  WITH_SPIN_LOCK (&state->lock) {
    if (pl011_rready(state)) {
      ringbuf_putb(&state->rx_buf, pl011_getc(state));
      cv_signal(&state->rx_nonempty);
      res = IF_FILTERED;
    }

    if (pl011_wready(state)) {
      uint8_t byte;
      if (ringbuf_getb(&state->tx_buf, &byte)) {
        pl011_putc(state, byte);
        cv_signal(&state->tx_nonfull);
      }
      res = IF_FILTERED;
    }
  }

  return res;
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
  cv_init(&state->rx_nonempty, "UART receive buffer not empty");
  cv_init(&state->tx_nonfull, "UART transmit buffer not full");

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
  devfs_makedev(NULL, "uart", &dev_uart_ops, state, NULL);

  return 0;
}

/* clang-format off */
static driver_t pl011_driver = {
  .desc = "PL011 UART driver",
  .size = sizeof(pl011_state_t),
  .attach = pl011_attach,
  .probe = pl011_probe,
};
/* clang-format on */

DEVCLASS_ENTRY(root, pl011_driver);
