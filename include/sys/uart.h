#ifndef _SYS_UART_H_
#define _SYS_UART_H_

#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/interrupt.h>
#include <sys/tty.h>
#include <sys/uart_tty.h>

typedef uint8_t (*uart_getc_t)(void *state);
typedef bool (*uart_rx_ready_t)(void *state);

typedef void (*uart_putc_t)(void *state, uint8_t byte);
typedef bool (*uart_tx_ready_t)(void *state);
typedef void (*uart_tx_enable_t)(void *state);
typedef void (*uart_tx_disable_t)(void *state);

typedef struct uart_methods {
  uart_getc_t getc;
  uart_rx_ready_t rx_ready;
  uart_putc_t putc;
  uart_tx_ready_t tx_ready;
  uart_tx_enable_t tx_enable;
  uart_tx_disable_t tx_disable;
} uart_methods_t;

#define UART_METHODS(dev)                                                      \
  (*(uart_methods_t *)(dev)->driver->interfaces[DIF_UART])

typedef struct uart_state {
  spin_t u_lock;
  ringbuf_t u_rx_buf;
  ringbuf_t u_tx_buf;
  tty_thread_t u_ttd;
  void *u_state;
} uart_state_t;

static inline uint8_t uart_getc(device_t *dev) {
  uart_methods_t methods = UART_METHODS(dev);
  uart_state_t *uart = dev->state;
  return methods.getc(uart->u_state);
}

static inline bool uart_rx_ready(device_t *dev) {
  uart_methods_t methods = UART_METHODS(dev);
  uart_state_t *uart = dev->state;
  return methods.rx_ready(uart->u_state);
}

static inline void uart_putc(device_t *dev, uint8_t byte) {
  uart_methods_t methods = UART_METHODS(dev);
  uart_state_t *uart = dev->state;
  methods.putc(uart->u_state, byte);
}

static inline bool uart_tx_ready(device_t *dev) {
  uart_methods_t methods = UART_METHODS(dev);
  uart_state_t *uart = dev->state;
  return methods.tx_ready(uart->u_state);
}

static inline void uart_tx_enable(device_t *dev) {
  uart_methods_t methods = UART_METHODS(dev);
  uart_state_t *uart = dev->state;
  methods.tx_enable(uart->u_state);
}

static inline void uart_tx_disable(device_t *dev) {
  uart_methods_t methods = UART_METHODS(dev);
  uart_state_t *uart = dev->state;
  methods.tx_disable(uart->u_state);
}

void uart_init(device_t *dev, const char *name, size_t buf_size, void *state,
               tty_t *tty);

intr_filter_t uart_intr(void *data /* device_t* */);

#endif /* _SYS_UART_H_ */
