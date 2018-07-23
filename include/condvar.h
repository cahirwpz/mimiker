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
/*! \brief Initialize a condvar.
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

/*! \brief Wait on a conditional variable. */
void cv_wait(condvar_t *cv, mtx_t *mtx);

/*! \brief Wait on a conditional variable with possiblity of being interrupted.
 *
 * \returns 0 if the thread was awoken normally (`cv_signal` or `cv_broadcast`)
 * \returns EINTR if the thread was interrupted during the sleep
 */
#define cv_wait_intr(cv, mtx) cv_wait_timed((cv), (mtx), 0)

/*! \brief Wait on a conditional variable with timeout.
 *
 * Timed sleep is also interruptible. The only guarantee about timeout is that
 * the thread's sleep will not timeout before the given time passes. It may
 * happen any time after that point.
 *
 * \arg timeout The timeout in system ticks, if 0 waits indefinitely.
 *
 * \returns 0 if the thread was awoken normally (`cv_signal`, `cv_broadcast`)
 * \returns EINTR if the thread was interrupted by a signal during the sleep
 * \returns ETIMEDOUT if the sleep timed out.
 */
int cv_wait_timed(condvar_t *cv, mtx_t *mtx, systime_t timeout);

/*! \brief Wake a single thread waiting on the condvar.
 *
 * If there are multiple waiting threads then the one with the highest priority
 * will be woken up.
 *
 * \sa sleepq_signal */
void cv_signal(condvar_t *cv);

/*! \brief Wake all threads waiting on a conditional variable.
 *
 * \sa sleepq_broadcast */
void cv_broadcast(condvar_t *cv);

#endif /* !_SYS_CONDVAR_H_ */
