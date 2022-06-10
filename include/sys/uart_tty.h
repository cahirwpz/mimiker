#ifndef _SYS_UART_TTY_H_
#define _SYS_UART_TTY_H_

#include <sys/condvar.h>
#include <sys/device.h>
#include <sys/tty.h>

/* tty->t_outq -> uart_state::u_tx_buf */
#define TTY_THREAD_TXRDY 0x1
/* uart_state::u_rx_buf -> tty->t_inq */
#define TTY_THREAD_RXRDY 0x2
#define TTY_THREAD_WORK_MASK (TTY_THREAD_TXRDY | TTY_THREAD_RXRDY)
/* If cleared, don't wake up worker thread from uart_intr. */
#define TTY_THREAD_OUTQ_NONEMPTY 0x4

typedef struct tty_thread {
  thread_t *ttd_thread;
  tty_t *ttd_tty;
  condvar_t ttd_cv; /* Use with uart_state::u_lock. */
  uint8_t ttd_flags;
} tty_thread_t;

void uart_tty_notify_out(tty_t *tty);

void uart_tty_thread_create(const char *name, device_t *dev, tty_t *tty);

#endif /* !_SYS_UART_TTY_H_ */
