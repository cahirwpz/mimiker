#define KL_LOG KL_DEV
#include <sys/bus.h>
#include <sys/devclass.h>
#include <sys/dtb.h>
#include <sys/fdt.h>
#include <sys/klog.h>
#include <sys/sched.h>
#include <sys/thread.h>
#include <dev/uart.h>

/*
 * CSR definitions.
 */
#define LITEUART_CSR_RXTX 0x00
#define LITEUART_CSR_TXFULL 0x04
#define LITEUART_CSR_RXEMPTY 0x08
#define LITEUART_CSR_EV_STATUS 0x0c
#define LITEUART_CSR_EV_PENDING 0x10
#define LITEUART_CSR_EV_ENABLE 0x14

/*
 * Event definitions.
 */
#define LITEUART_EV_TX 1
#define LITEUART_EV_RX 2

#define LITEUART_BUFSIZE 128

typedef struct liteuart_state {
  resource_t *csrs;
  thread_t *thread;
} liteuart_state_t;

/*
 * CSR access macros.
 */
#define csr_read(csr) bus_read_4(liteuart->csrs, (csr))
#define csr_write(csr, v) bus_write_4(liteuart->csrs, (csr), (v))

static bool liteuart_rx_ready(void *state) {
  liteuart_state_t *liteuart = state;
  return csr_read(LITEUART_CSR_RXEMPTY) != 0;
}

static uint8_t liteuart_getc(void *state) {
  liteuart_state_t *liteuart = state;
  uint8_t c = csr_read(LITEUART_CSR_RXTX);
  csr_write(LITEUART_CSR_EV_PENDING, LITEUART_EV_TX | LITEUART_EV_RX);
  return c;
}

static bool liteuart_tx_ready(void *state) {
  liteuart_state_t *liteuart = state;
  return csr_read(LITEUART_CSR_TXFULL) == 0;
}

static void liteuart_putc(void *state, uint8_t c) {
  liteuart_state_t *liteuart = state;
  return csr_write(LITEUART_CSR_RXTX, c);
}

static void liteuart_tx_enable(void *state __unused) {
  /* Nothing to do here. */
}

static void liteuart_tx_disable(void *state __unused) {
  /* Nothing to do here. */
}

static int liteuart_probe(device_t *dev) {
  void *dtb = dtb_root();
  const char *compatible = fdt_getprop(dtb, dev->node, "compatible", NULL);
  if (!compatible)
    return 0;
  return strcmp(compatible, "litex,liteuart") == 0;
}

static int liteuart_attach(device_t *dev) {
  liteuart_state_t *liteuart =
    kmalloc(M_DEV, sizeof(liteuart_state_t), M_WAITOK | M_ZERO);
  assert(liteuart);

  tty_t *tty = tty_alloc();
  tty->t_termios.c_ispeed = 115200;
  tty->t_termios.c_ospeed = 115200;
  tty->t_ops.t_notify_out = uart_tty_notify_out;

  uart_init(dev, "liteuart", LITEUART_BUFSIZE, liteuart, tty);

  liteuart->csrs = device_take_memory(dev, 0, RF_ACTIVE);
  assert(liteuart->csrs);

  /* Disable events. */
  csr_write(LITEUART_CSR_EV_ENABLE, 0);

  /* Create a thread for polling. */
  liteuart->thread = thread_create("liteuart poller", uart_thread, dev,
                                   prio_ithread(PRIO_ITHRD_QTY - 1));
  sched_add(liteuart->thread);

  /* Prepare /dev/uart interface. */
  tty_makedev(NULL, "uart", tty);

  return 0;
}

static uart_methods_t liteuart_methods = {
  .rx_ready = liteuart_rx_ready,
  .getc = liteuart_getc,
  .tx_ready = liteuart_tx_ready,
  .putc = liteuart_putc,
  .tx_enable = liteuart_tx_enable,
  .tx_disable = liteuart_tx_disable,
};

static driver_t liteuart_driver = {
  .desc = "liteuart",
  .size = sizeof(uart_state_t),
  .pass = SECOND_PASS,
  .probe = liteuart_probe,
  .attach = liteuart_attach,
  .interfaces =
    {
      [DIF_UART] = &liteuart_methods,
    },
};

DEVCLASS_ENTRY(root, liteuart_driver);
