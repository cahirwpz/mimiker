#ifndef _SYS_CONDVAR_H_
#define _SYS_CONDVAR_H_

#include <common.h>

typedef struct condvar {
  const char *name;
  int waiters;
} condvar_t;

typedef struct mtx mtx_t;

void cv_init(condvar_t *cv, const char *name);

/* TODO: Make conditional variable unusable after it has been destroyed. */
#define cv_destroy(m)

void cv_wait(condvar_t *cv, mtx_t *mtx);

/* \brief Wait interruptibly on a condvar.
 *
 * Interruptible sleep may be interrupted by incoming signals.
 *
 * \returns 0 if the thread was awoken normally (`sleepq_signal`, `_broadcast`)
 * \returns EINTR if the thread was interrupted by a signal during the sleep
 */
int cv_wait_intr(condvar_t *cv, mtx_t *mtx);

/* \brief Wait on a condvar with timeout.
 *
 * Timed sleep is also interruptible. The only guarantee about timeout is that
 * the thread's sleep will not timeout before the given time passes. It may
 * happen any time after that point.
 *
 * \arg timeout The timeout in system ticks. One tick is 1ms as of writing.
 *
 * \returns 0 if the thread was awoken normally (`sleepq_signal`, `_broadcast`)
 * \returns EINTR if the thread was interrupted by a signal during the sleep
 * \returns ETIMEDOUT if the sleep timed out.
 */
int cv_wait_timed(condvar_t *cv, mtx_t *mtx, systime_t timeout);

void cv_signal(condvar_t *cv);
void cv_broadcast(condvar_t *cv);

#endif /* !_SYS_CONDVAR_H_ */
