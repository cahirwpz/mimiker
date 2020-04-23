#ifndef _SYS_CONDVAR_H_
#define _SYS_CONDVAR_H_

#include <sys/types.h>
#include <sys/lock.h>

typedef struct condvar {
  const char *name;     /*!< name for debugging purpose */
  volatile int waiters; /*!< # of threads sleeping in associated sleep queue */
} condvar_t;

/*! \brief Initialize a conditional variable.
 *
 * Any condvar must be initialized before use.
 *
 * \arg cv Pointer to the structure that's going to be initialized.
 * \arg name Pointer to statically (!) allocated string.
 */
void cv_init(condvar_t *cv, const char *name);

/*! \brief Make conditional variable unusable after it has been destroyed.
 * XXX Do we really need it?
 */
#define cv_destroy(m)

/*! \brief Wait on a conditional variable. */
void cv_wait(condvar_t *cv, lock_t m);

/*! \brief Wait on a conditional variable with possiblity of being interrupted.
 *
 * \returns 0 if the thread was awoken normally (`cv_signal` or `cv_broadcast`)
 * \returns EINTR if the thread was interrupted during the sleep
 */
#define cv_wait_intr(cv, m) cv_wait_timed((cv), (m), 0)

/*! \brief Wait on a conditional variable with timeout.
 *
 * \arg timeout The timeout in system ticks, if 0 waits indefinitely.
 *
 * \returns 0 if the thread was awoken normally (`cv_signal`, `cv_broadcast`)
 * \returns EINTR if the thread was interrupted during the sleep
 * \returns ETIMEDOUT if the sleep timed out
 */
int cv_wait_timed(condvar_t *cv, lock_t m, systime_t timeout);

/*! \brief Wake a single thread waiting on a conditional variable.
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
