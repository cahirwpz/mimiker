#define KL_LOG KL_DEV
#include <sys/bus.h>
#include <sys/devclass.h>
#include <sys/fdt.h>
#include <sys/klog.h>
#include <sys/sched.h>
#include <sys/thread.h>
#include <dev/uart.h>

#define SFUART_TXDATA 0x00
#define SFUART_TXDATA_FULL (1 << 31)

#define SFUART_RXDATA 0x04
#define SFUART_RXDATA_DATAMASK 0xff
#define SFUART_RXDATA_EMPTY (1 << 31)

#define SFUART_TXCTRL 0x08
#define SFUART_TXCTRL_ENABLE 0x01
#define SFUART_TXCTRL_NSTOP 0x02
#define SFUART_TXCTRL_TXCNT 0x70000
#define SFUART_TXCTRL_TXCNT_SHIFT 16

#define SFUART_RXCTRL 0x0c
#define SFUART_RXCTRL_ENABLE 0x01
#define SFUART_RXCTRL_RXCNT 0x70000
#define SFUART_RXCTRL_RXCNT_SHIFT 16

#define SFUART_IRQ_ENABLE 0x10
#define SFUART_IRQ_ENABLE_TXWM 0x01
#define SFUART_IRQ_ENABLE_RXWM 0x02

#define SFUART_IRQ_PENDING 0x14
#define SFUART_IRQ_PENDING_TXWM 0x01
#define SFUART_IRQ_PENDING_RXQM 0x02

#define SFUART_DIV 0x18

typedef struct sfuart_state {
  resource_t *regs;
  resource_t *irq;
  uint8_t c;
  bool buffered;
} sfuart_state_t;

#define in(r) bus_read_4(sfuart->regs, (r))
#define out(r, v) bus_write_4(sfuart->regs, (r), (v))

#define set(r, b) out((r), in(r) | (b))
#define clr(r, b) out((r), in(r) & ~(b))

#define SFUART_BUFSIZE 128

static bool sfuart_rx_ready(void *state) {
  sfuart_state_t *sfuart = state;
  uint32_t data = in(SFUART_RXDATA);
  if (data & SFUART_RXDATA_EMPTY)
    return false;
  sfuart->c = data & SFUART_RXDATA_DATAMASK;
  sfuart->buffered = true;
  return true;
}

static uint8_t sfuart_getc(void *state) {
  sfuart_state_t *sfuart = state;
  if (sfuart->buffered) {
    sfuart->buffered = false;
    return sfuart->c;
  }
  return in(SFUART_RXDATA)&SFUART_RXDATA_DATAMASK;
}

static bool sfuart_tx_ready(void *state) {
  sfuart_state_t *sfuart = state;
  return !(in(SFUART_TXDATA)&SFUART_TXDATA_FULL);
}

static void sfuart_putc(void *state, uint8_t c) {
  sfuart_state_t *sfuart = state;
  out(SFUART_TXDATA, c);
}

static void sfuart_tx_enable(void *state) {
  sfuart_state_t *sfuart = state;
  set(SFUART_IRQ_ENABLE, SFUART_IRQ_ENABLE_TXWM);
}

static void sfuart_tx_disable(void *state) {
  sfuart_state_t *sfuart = state;
  clr(SFUART_IRQ_ENABLE, SFUART_IRQ_ENABLE_TXWM);
}

static int sfuart_probe(device_t *dev) {
  return FDT_is_compatible(dev->node, "sifive,uart0");
}

static int sfuart_attach(device_t *dev) {
  sfuart_state_t *sfuart =
    kmalloc(M_DEV, sizeof(sfuart_state_t), M_WAITOK | M_ZERO);
  int err = 0;

  tty_t *tty = tty_alloc();
  tty->t_termios.c_ispeed = 115200;
  tty->t_termios.c_ospeed = 115200;
  tty->t_ops.t_notify_out = uart_tty_notify_out;

  sfuart->regs = device_take_memory(dev, 0);
  assert(sfuart->regs);

  if ((err = bus_map_resource(dev, sfuart->regs)))
    return err;

  uart_init(dev, "SiFive UART", SFUART_BUFSIZE, sfuart, tty);

  out(SFUART_IRQ_ENABLE, 0);

  out(SFUART_RXCTRL, (0 << SFUART_RXCTRL_RXCNT_SHIFT) | SFUART_RXCTRL_ENABLE);
  out(SFUART_TXCTRL, (1 << SFUART_TXCTRL_TXCNT_SHIFT) | SFUART_TXCTRL_ENABLE);

  out(SFUART_IRQ_ENABLE, SFUART_IRQ_ENABLE_RXWM);

  sfuart->irq = device_take_irq(dev, 0);
  assert(sfuart->irq);

  pic_setup_intr(dev, sfuart->irq, uart_intr, NULL, dev, "SiFive UART");

  /* Prepare /dev/uart interface. */
  tty_makedev(NULL, "uart", tty);

  return 0;
}

static uart_methods_t sfuart_methods = {
  .rx_ready = sfuart_rx_ready,
  .getc = sfuart_getc,
  .tx_ready = sfuart_tx_ready,
  .putc = sfuart_putc,
  .tx_enable = sfuart_tx_enable,
  .tx_disable = sfuart_tx_disable,
};

static driver_t sfuart_driver = {
  .desc = "SiFive UART",
  .size = sizeof(uart_state_t),
  .pass = SECOND_PASS,
  .probe = sfuart_probe,
  .attach = sfuart_attach,
  .interfaces =
    {
      [DIF_UART] = &sfuart_methods,
    },
};

DEVCLASS_ENTRY(root, sfuart_driver);
