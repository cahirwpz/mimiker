#include <sys/spinlock.h>
#include <sys/ringbuf.h>
#include <sys/uart.h>

void uart_init(device_t *dev, const char *name, size_t buf_size, void *state,
               tty_t *tty) {
  uart_state_t *uart = dev->state;
  uart->u_state = state;
  uart->u_tty = tty;
  tty->t_data = dev;

  ringbuf_init(&uart->u_rx_buf, kmalloc(M_DEV, buf_size, M_ZERO), buf_size);
  ringbuf_init(&uart->u_tx_buf, kmalloc(M_DEV, buf_size, M_ZERO), buf_size);

  spin_init(&uart->u_lock, 0);
  tty_thread_create(name, dev);
}

bool uart_getb_lock(uart_state_t *uart, uint8_t *byte_p) {
  SCOPED_SPIN_LOCK(&uart->u_lock);
  return ringbuf_getb(&uart->u_rx_buf, byte_p);
}

/*
 * If tx_buf is empty, we can try to write characters directly from tty->t_outq.
 * This routine attempts to do just that.
 * Must be called with both tty->t_lock and uart->lock held.
 */
static void uart_try_bypass_txbuf(device_t *dev) {
  uart_state_t *uart = dev->state;
  tty_t *tty = uart->u_tty;
  uint8_t byte;

  if (!ringbuf_empty(&uart->u_tx_buf))
    return;

  while (uart_tx_ready(dev) && ringbuf_getb(&tty->t_outq, &byte))
    uart_putc(dev, byte);
}

/*
 * Move characters from tty->t_outq to uart->tx_buf.
 * Must be called with tty->t_lock held.
 */
void uart_fill_txbuf(device_t *dev) {
  uart_state_t *uart = dev->state;
  tty_t *tty = uart->u_tty;
  uint8_t byte;

  while (true) {
    SCOPED_SPIN_LOCK(&uart->u_lock);
    uart_try_bypass_txbuf(dev);
    if (ringbuf_full(&uart->u_tx_buf) || !ringbuf_getb(&tty->t_outq, &byte)) {
      /* Enable TXRDY interrupts if there are characters in tx_buf. */
      if (!ringbuf_empty(&uart->u_tx_buf))
        uart_tx_enable(dev);
      tty_set_outq_nonempty_flag(&uart->u_ttd, tty);
      break;
    }
    tty_set_outq_nonempty_flag(&uart->u_ttd, tty);
    ringbuf_putb(&uart->u_tx_buf, byte);
  }
  tty_getc_done(tty);
}

intr_filter_t uart_intr(void *data /* device_t* */) {
  device_t *dev = data;
  uart_state_t *uart = dev->state;
  tty_thread_t *ttd = &uart->u_ttd;
  intr_filter_t res = IF_STRAY;

  WITH_SPIN_LOCK (&uart->u_lock) {
    if (uart_rx_ready(dev)) {
      (void)ringbuf_putb(&uart->u_rx_buf, uart_getc(dev));
      ttd->ttd_flags |= TTY_THREAD_RXRDY;
      cv_signal(&ttd->ttd_cv);
      res = IF_FILTERED;
    }

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

/*
 * New characters have appeared in the tty's output queue.
 * Fill the UART's tx_buf and enable TXRDY interrupts.
 * Called with `tty->t_lock` held.
 */
void uart_notify_out(tty_t *tty) {
  device_t *dev = tty->t_data;

  if (ringbuf_empty(&tty->t_outq))
    return;

  uart_fill_txbuf(dev);
}
