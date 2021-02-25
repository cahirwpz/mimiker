#ifndef _SYS_UART_TTY_H_
#define _SYS_UART_TTY_H_

#include <sys/condvar.h>
#include <sys/device.h>
#include <sys/tty.h>

#define TTY_THREAD_TXRDY 0x1
#define TTY_THREAD_RXRDY 0x2
#define TTY_THREAD_WORK_MASK (TTY_THREAD_TXRDY | TTY_THREAD_RXRDY)
#define TTY_THREAD_OUTQ_NONEMPTY 0x4

typedef struct tty_thread {
  thread_t *ttd_thread;
  tty_t *ttd_tty;
  condvar_t ttd_cv;
  uint8_t ttd_flags;
} tty_thread_t;

void uart_tty_notify_out(tty_t *tty);

void uart_tty_thread_create(const char *name, device_t *dev, tty_t *tty);

#endif /* !_SYS_UART_TTY_H_ */
