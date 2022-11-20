#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/bus.h>
#include <sys/devclass.h>
#include <sys/vnode.h>
#include <sys/devfs.h>
#include <sys/stat.h>
#include <sys/libkern.h>
#include <sys/ttycom.h>
#include <sys/interrupt.h>
#include <sys/ringbuf.h>
#include <sys/tty.h>
#include <sys/fdt.h>
#include <dev/uart.h>
#include <sys/uart_tty.h>
#include <dev/bcm2835reg.h>
#include <dev/plcomreg.h>

#define UART0_BASE BCM2835_PERIPHERALS_BUS_TO_PHYS(BCM2835_UART0_BASE)
#define UART_BUFSIZE 128

static inline void set4(dev_mem_t *mem, int o, uint32_t v) {
  bus_write_4(mem, o, bus_read_4(mem, o) | v);
}

static inline void clr4(dev_mem_t *mem, int o, uint32_t v) {
  bus_write_4(mem, o, bus_read_4(mem, o) & ~v);
}

typedef struct pl011_state {
  dev_mem_t *regs;
  dev_intr_t *irq;
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
  return FDT_is_compatible(dev->node, "arm,pl011") ||
         FDT_is_compatible(dev->node, "arm,primecell");
}

static int pl011_attach(device_t *dev) {
  pl011_state_t *pl011 = kmalloc(M_DEV, sizeof(pl011_state_t), M_ZERO);
  int err = 0;

  if ((err = device_claim_intr(dev, 0, uart_intr, NULL, dev, "PL011 UART",
                               &pl011->irq))) {
    goto end;
  }

  if ((err = device_claim_mem(dev, 0, &pl011->regs)))
    goto end;

  tty_t *tty = tty_alloc();
  tty->t_termios.c_ispeed = 115200;
  tty->t_termios.c_ospeed = 115200;
  tty->t_ops.t_notify_out = uart_tty_notify_out;

  uart_init(dev, "pl011", UART_BUFSIZE, pl011, tty);

  /* Disable UART0. */
  bus_write_4(pl011->regs, PL011COM_CR, 0);
  /* Clear pending interrupts. */
  bus_write_4(pl011->regs, PL011COM_ICR, PL011_INT_ALLMASK);

  /* TODO(pj) do magic with mail buffer */

  /*
   * Set integer & fractional part of baud rate.
   * Divider = UART_CLOCK/(16 * Baud)
   * Fraction part register = (Fractional part * 64) + 0.5
   * UART_CLOCK = 3000000; Baud = 115200.
   *
   * Divider = 4000000/(16 * 115200) = 2.17 = ~2.
   * Fractional part register = (.17 * 64) + 0.5 = 11.3 = ~11 = 0xb.
   */

  bus_write_4(pl011->regs, PL011COM_IBRD, 2);
  bus_write_4(pl011->regs, PL011COM_FBRD, 0xb);

  /* Enable FIFO & 8 bit data transmission (1 stop bit, no parity). */
  bus_write_4(pl011->regs, PL011COM_LCRH, PL01X_LCR_FEN | PL01X_LCR_8BITS);

  /* Mask all interrupts. */
  bus_write_4(pl011->regs, PL011COM_IMSC, PL011_INT_ALLMASK);

  /* Enable UART0, receive & transfer part of UART. */
  bus_write_4(pl011->regs, PL011COM_CR,
              PL01X_CR_UARTEN | PL011_CR_TXE | PL011_CR_RXE);

  /* Enable interrupt. */
  bus_write_4(pl011->regs, PL011COM_IMSC, PL011_INT_RX);

  /* Prepare /dev/uart interface. */
  tty_makedev(NULL, "uart", tty);

end:
  if (err)
    kfree(M_DEV, pl011);
  return err;
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
