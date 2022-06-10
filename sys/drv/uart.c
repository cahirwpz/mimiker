#include <sys/spinlock.h>
#include <sys/ringbuf.h>
#include <dev/uart.h>
#include <sys/uart_tty.h>

void uart_init(device_t *dev, const char *name, size_t buf_size, void *state,
               tty_t *tty) {
  uart_state_t *uart = dev->state;
  uart->u_state = state;

  ringbuf_init(&uart->u_rx_buf, kmalloc(M_DEV, buf_size, M_ZERO), buf_size);
  ringbuf_init(&uart->u_tx_buf, kmalloc(M_DEV, buf_size, M_ZERO), buf_size);

  spin_init(&uart->u_lock, 0);
  uart_tty_thread_create(name, dev, tty);
}

intr_filter_t uart_intr(void *data /* device_t* */) {
  device_t *dev = data;
  uart_state_t *uart = dev->state;
  tty_thread_t *ttd = &uart->u_ttd;
  intr_filter_t res = IF_STRAY;

  WITH_SPIN_LOCK (&uart->u_lock) {
    /* data ready to be received? */
    if (uart_rx_ready(dev)) {
      (void)ringbuf_putb(&uart->u_rx_buf, uart_getc(dev));
      ttd->ttd_flags |= TTY_THREAD_RXRDY;
      cv_signal(&ttd->ttd_cv);
      res = IF_FILTERED;
    }

    /* transmit register empty? */
    if (uart_tx_ready(dev)) {
      uint8_t byte;
      while (uart_tx_ready(dev) && ringbuf_getb(&uart->u_tx_buf, &byte))
        uart_putc(dev, byte);
      if (ringbuf_empty(&uart->u_tx_buf)) {
        /* If we're out of characters and there are characters
         * in the tty's output queue, signal the tty thread to refill. */
        if (ttd->ttd_flags & TTY_THREAD_OUTQ_NONEMPTY) {
          ttd->ttd_flags |= TTY_THREAD_TXRDY;
          cv_signal(&ttd->ttd_cv);
        }
        /* Disable TXRDY interrupts - the tty thread will re-enable them
         * after filling tx_buf. */
        uart_tx_disable(dev);
      }
      res = IF_FILTERED;
    }
  }

  return res;
}
