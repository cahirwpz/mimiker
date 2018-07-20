#ifndef _SYS_CONDVAR_H_
#define _SYS_CONDVAR_H_

#include <common.h>

/* \todo document this? */
typedef struct condvar {
  const char *name;
  int waiters;
} condvar_t;

typedef struct mtx mtx_t;

/* \todo is the block alignment correct for Doxygen?
 * \todo should we mention that none of the functions here will deallocate
 *       the string? (in case the string is dynamically allocated but it
 *       doesn't really have a reason to be)
 */
/* \brief Initialize a condvar.
 *
 * Any condvar must be initialized before use.
 *
 * \arg name Name of the condvar for debugging purposes. Only the pointer to
 *           the string is stored so don't modify the string after passing it
 *           to a condvar.
 * \arg cv Pointer to the structure that's initialized.
 */
void cv_init(condvar_t *cv, const char *name);

/* \brief I don't know what it's supposed to do because it does nothing.
 * \todo Make conditional variable unusable after it has been destroyed.
 */
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

/* \brief Wake a single thread waiting on the condvar.
 *
 * If there are multiple waiting threads then the one with the highest priority
 * will be woken up (dependence on `sleepq_signal`).
 */
void cv_signal(condvar_t *cv);

/* \brief Wake all threads waiting on the condvar.
 */
void cv_broadcast(condvar_t *cv);

#endif /* !_SYS_CONDVAR_H_ */
