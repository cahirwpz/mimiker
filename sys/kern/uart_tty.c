#include <sys/sched.h>
#include <sys/tty.h>
#include <dev/uart.h>
#include <sys/uart_tty.h>

static void tty_set_outq_nonempty_flag(tty_thread_t *ttd) {
  if (ringbuf_empty(&ttd->ttd_tty->t_outq))
    ttd->ttd_flags &= ~TTY_THREAD_OUTQ_NONEMPTY;
  else
    ttd->ttd_flags |= TTY_THREAD_OUTQ_NONEMPTY;
}

static bool uart_getb_lock(uart_state_t *uart, uint8_t *byte_p) {
  SCOPED_SPIN_LOCK(&uart->u_lock);
  return ringbuf_getb(&uart->u_rx_buf, byte_p);
}

/*
 * If tx_buf is empty, we can try to write characters directly from tty->t_outq.
 * This routine attempts to do just that.
 * Must be called with both tty->t_lock and uart->lock held.
 */
static void uart_tty_try_bypass_txbuf(device_t *dev) {
  uart_state_t *uart = dev->state;
  tty_t *tty = uart->u_ttd.ttd_tty;
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
static void uart_tty_fill_txbuf(device_t *dev) {
  uart_state_t *uart = dev->state;
  tty_t *tty = uart->u_ttd.ttd_tty;
  uint8_t byte;

  while (true) {
    SCOPED_SPIN_LOCK(&uart->u_lock);
    uart_tty_try_bypass_txbuf(dev);
    if (ringbuf_full(&uart->u_tx_buf) || !ringbuf_getb(&tty->t_outq, &byte)) {
      /* Enable TXRDY interrupts if there are characters in tx_buf. */
      if (!ringbuf_empty(&uart->u_tx_buf))
        uart_tx_enable(dev);
      tty_set_outq_nonempty_flag(&uart->u_ttd);
      break;
    }
    tty_set_outq_nonempty_flag(&uart->u_ttd);
    ringbuf_putb(&uart->u_tx_buf, byte);
  }
  tty_getc_done(tty);
}

/*
 * New characters have appeared in the tty's output queue.
 * Fill the UART's tx_buf and enable TXRDY interrupts.
 * Called with `tty->t_lock` held.
 */
void uart_tty_notify_out(tty_t *tty) {
  device_t *dev = tty->t_data;

  if (ringbuf_empty(&tty->t_outq))
    return;

  uart_tty_fill_txbuf(dev);
}

/* TODO: revisit after per-intr_event ithreads are implemented. */
static void uart_tty_thread(void *arg) {
  device_t *dev = arg;
  uart_state_t *uart = dev->state;
  tty_thread_t *ttd = &uart->u_ttd;
  tty_t *tty = ttd->ttd_tty;
  uint8_t work, byte;

  while (true) {
    WITH_SPIN_LOCK (&uart->u_lock) {
      /* Sleep until there's work for us to do. */
      while ((work = ttd->ttd_flags & TTY_THREAD_WORK_MASK) == 0)
        cv_wait(&ttd->ttd_cv, &uart->u_lock);
      ttd->ttd_flags &= ~TTY_THREAD_WORK_MASK;
    }
    WITH_MTX_LOCK (&tty->t_lock) {
      if (work & TTY_THREAD_RXRDY) {
        /* Move characters from rx_buf into the tty's input queue. */
        while (uart_getb_lock(uart, &byte))
          if (!tty_input(tty, byte))
            klog("dropped character %hhx", byte);
      }
      if (work & TTY_THREAD_TXRDY)
        uart_tty_fill_txbuf(dev);
    }
  }
}

void uart_tty_thread_create(const char *name, device_t *dev, tty_t *tty) {
  uart_state_t *uart = dev->state;
  tty_thread_t *ttd = &uart->u_ttd;
  ttd->ttd_tty = tty;
  tty->t_data = dev;
  cv_init(&ttd->ttd_cv, "TTY thread notification");
  ttd->ttd_thread =
    thread_create(name, uart_tty_thread, dev, prio_ithread(PRIO_ITHRD_QTY - 1));
  sched_add(ttd->ttd_thread);
}
