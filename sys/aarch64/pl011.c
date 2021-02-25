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
#include <sys/uart.h>
#include <sys/uart_tty.h>
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
  bus_write_4(r, o, bus_read_4(r, o) & ~v);
}

typedef struct pl011_state {
  resource_t *regs;
  resource_t *irq;
} pl011_state_t;

static bool pl011_rx_ready(void *state) {
  pl011_state_t *pl011 = state;
  return (bus_read_4(pl011->regs, PL01XCOM_FR) & PL01X_FR_RXFE) == 0;
}

static uint8_t pl011_getc(void *state) {
  pl011_state_t *pl011 = state;
  return bus_read_4(pl011->regs, PL01XCOM_DR);
}

static void pl011_putc(void *state, uint8_t c) {
  pl011_state_t *pl011 = state;
  bus_write_4(pl011->regs, PL01XCOM_DR, c);
}

static bool pl011_tx_ready(void *state) {
  pl011_state_t *pl011 = state;
  return (bus_read_4(pl011->regs, PL01XCOM_FR) & PL01X_FR_TXFF) == 0;
}

static void pl011_tx_enable(void *state) {
  pl011_state_t *pl011 = state;
  set4(pl011->regs, PL011COM_CR, PL011_CR_TXE);
}

static void pl011_tx_disable(void *state) {
  pl011_state_t *pl011 = state;
  clr4(pl011->regs, PL011COM_CR, PL011_CR_TXE);
}

static int pl011_probe(device_t *dev) {
  /* (pj) so far we don't have better way to associate driver with device for
   * buses which do not automatically enumerate their children. */
  return (dev->unit == 1);
}

static int pl011_attach(device_t *dev) {
  pl011_state_t *pl011 = kmalloc(M_DEV, sizeof(pl011_state_t), M_ZERO);

  tty_t *tty = tty_alloc();
  tty->t_termios.c_ispeed = 115200;
  tty->t_termios.c_ospeed = 115200;
  tty->t_ops.t_notify_out = uart_tty_notify_out;

  uart_init(dev, "pl011", UART_BUFSIZE, pl011, tty);

  resource_t *r = device_take_memory(dev, 0, 0);

  /* (pj) BCM2835_UART0_SIZE is much smaller than PAGESIZE */
  bus_space_map(r->r_bus_tag, resource_start(r), PAGESIZE, &r->r_bus_handle);

  assert(r != NULL);

  pl011->regs = r;

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

  pl011->irq = device_take_irq(dev, 0, RF_ACTIVE);
  bus_intr_setup(dev, pl011->irq, uart_intr, NULL, dev, "PL011 UART");

  /* Prepare /dev/uart interface. */
  tty_makedev(NULL, "uart", tty);

  return 0;
}

static uart_methods_t pl011_methods = {
  .getc = pl011_getc,
  .rx_ready = pl011_rx_ready,
  .putc = pl011_putc,
  .tx_ready = pl011_tx_ready,
  .tx_enable = pl011_tx_enable,
  .tx_disable = pl011_tx_disable,
};

static driver_t pl011_driver = {
  .desc = "PL011 UART driver",
  .size = sizeof(uart_state_t),
  .pass = SECOND_PASS,
  .attach = pl011_attach,
  .probe = pl011_probe,
  .interfaces =
    {
      [DIF_UART] = &pl011_methods,
    },
};

DEVCLASS_ENTRY(root, pl011_driver);
