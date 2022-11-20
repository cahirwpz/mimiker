#define KL_LOG KL_DEV
#include <sys/libkern.h>
#include <sys/vnode.h>
#include <sys/devfs.h>
#include <sys/klog.h>
#include <sys/condvar.h>
#include <sys/ringbuf.h>
#include <sys/bus.h>
#include <sys/termios.h>
#include <sys/ttycom.h>
#include <dev/isareg.h>
#include <dev/ns16550reg.h>
#include <sys/stat.h>
#include <sys/devclass.h>
#include <sys/tty.h>
#include <dev/uart.h>
#include <sys/uart_tty.h>

#define UART_BUFSIZE 128

typedef struct ns16550_state {
  dev_intr_t *irq_res;
  dev_mem_t *regs;
} ns16550_state_t;

#define in(regs, offset) bus_read_1((regs), (offset))
#define out(regs, offset, value) bus_write_1((regs), (offset), (value))

static void set(dev_mem_t *regs, unsigned offset, uint8_t mask) {
  out(regs, offset, in(regs, offset) | mask);
}

static void clr(dev_mem_t *regs, unsigned offset, uint8_t mask) {
  out(regs, offset, in(regs, offset) & ~mask);
}

static void setup(dev_mem_t *regs) {
  set(regs, LCR, LCR_DLAB);
  out(regs, DLM, 0);
  out(regs, DLL, 1); /* 115200 */
  clr(regs, LCR, LCR_DLAB);

  out(regs, IER, 0);
  out(regs, FCR, 0);
  out(regs, LCR, LCR_8BITS); /* 8-bit data, no parity */
}

static uint8_t ns16550_getc(void *state) {
  ns16550_state_t *ns16550 = state;
  return in(ns16550->regs, RBR);
}

static bool ns16550_rx_ready(void *state) {
  ns16550_state_t *ns16550 = state;
  return in(ns16550->regs, IIR) & IIR_RXRDY;
}

static void ns16550_putc(void *state, uint8_t byte) {
  ns16550_state_t *ns16550 = state;
  out(ns16550->regs, THR, byte);
}

static bool ns16550_tx_ready(void *state) {
  ns16550_state_t *ns16550 = state;
  return in(ns16550->regs, LSR) & LSR_THRE;
}

static void ns16550_tx_enable(void *state) {
  ns16550_state_t *ns16550 = state;
  set(ns16550->regs, IER, IER_ETXRDY);
}

static void ns16550_tx_disable(void *state) {
  ns16550_state_t *ns16550 = state;
  clr(ns16550->regs, IER, IER_ETXRDY);
}

static int ns16550_attach(device_t *dev) {
  ns16550_state_t *ns16550 = kmalloc(M_DEV, sizeof(ns16550_state_t), M_ZERO);
  int err = 0;

  if ((err = device_claim_intr(dev, 0, uart_intr, NULL, dev, "NS16550 UART",
                               &ns16550->irq_res))) {
    goto end;
  }

  if ((err = device_claim_mem(dev, 0, &ns16550->regs)))
    goto end;

  tty_t *tty = tty_alloc();
  tty->t_termios.c_ispeed = 115200;
  tty->t_termios.c_ospeed = 115200;
  tty->t_ops.t_notify_out = uart_tty_notify_out;

  uart_init(dev, "ns16550", UART_BUFSIZE, ns16550, tty);

  /* Setup UART and enable interrupts */
  setup(ns16550->regs);
  out(ns16550->regs, IER, IER_ERXRDY | IER_ETXRDY);

  /* Prepare /dev/uart interface. */
  tty_makedev(NULL, "uart", tty);

end:
  if (err)
    kfree(M_DEV, ns16550);
  return err;
}

static int ns16550_probe(device_t *dev) {
  return dev->unit == 1;
}

static uart_methods_t ns16550_methods = {
  .getc = ns16550_getc,
  .rx_ready = ns16550_rx_ready,
  .putc = ns16550_putc,
  .tx_ready = ns16550_tx_ready,
  .tx_enable = ns16550_tx_enable,
  .tx_disable = ns16550_tx_disable,
};

static driver_t ns16550_driver = {
  .desc = "NS16550 UART driver",
  .size = sizeof(uart_state_t),
  .pass = SECOND_PASS,
  .attach = ns16550_attach,
  .probe = ns16550_probe,
  .interfaces =
    {
      [DIF_UART] = &ns16550_methods,
    },
};

DEVCLASS_ENTRY(isa, ns16550_driver);
