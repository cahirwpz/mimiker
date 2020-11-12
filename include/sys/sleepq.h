#ifndef _SYS_SLEEPQ_H_
#define _SYS_SLEEPQ_H_

#include <stdbool.h>
#include <sys/types.h>
#include <sys/queue.h>

typedef struct thread thread_t;
typedef struct sleepq sleepq_t;

/*! \file sleepq.h */

/*! \brief Called during kernel initialization. */
void init_sleepq(void);

/*! \brief Allocates sleep queue entry. */
sleepq_t *sleepq_alloc(void);

/*! \brief Deallocates sleep queue entry. */
void sleepq_destroy(sleepq_t *sq);

/*! \brief Blocks the current thread until it is awakened from its sleep queue.
 *
 * \param wchan unique sleep queue identifier
 * \param waitpt caller associated with sleep action
 */
void sleepq_wait(void *wchan, const void *waitpt);

/*! \brief Same as \a sleepq_wait but allows the sleep to be interrupted. */
#define sleepq_wait_intr(wchan, waitpt) sleepq_wait_timed((wchan), (waitpt), 0)

/*! \brief Performs interruptible sleep with timeout.
 *
 * Timed sleep is also interruptible. The only guarantee about timeout is that
 * the thread's sleep will not timeout before the given time passes. It may
 * happen any time after that point.
 *
 * \note Spurious wakeups can happen. The return value is 0 in that case.
 * \param timeout in system ticks, if 0 waits indefinitely
 * \returns how the thread was actually woken up */
int sleepq_wait_timed(void *wchan, const void *waitpt, systime_t timeout);

/*! \brief Wakes up highest priority thread waiting on \a wchan.
 *
 * \param wchan unique sleep queue identifier
 */
bool sleepq_signal(void *wchan);

/*! \brief Resume all threads sleeping on \a wchan.
 *
 * \param wchan unique sleep queue identifier
 */
bool sleepq_broadcast(void *wchan);

/*! \brief Break thread's sleep.
 *
 * \returns true on success
 * \returns false if the thread has not been asleep
 */
bool sleepq_abort(thread_t *td);

#endif /* !_SYS_SLEEPQ_H_ */
