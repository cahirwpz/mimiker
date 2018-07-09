#ifndef _SYS_SLEEPQ_H_
#define _SYS_SLEEPQ_H_

#include <common.h>
#include <queue.h>

typedef struct thread thread_t;
typedef struct sleepq sleepq_t;

/*! \file sleepq.h */

/* Used to return reason of wakeup from _sleepq_wait. */
typedef enum { SQ_NORMAL = 0, SQ_ABORT = 1, SQ_TIME = 2 } sq_wakeup_t;

/*! \brief Initializes sleep queues.
 *
 * \warning To be called only from early kernel initialization! */
void sleepq_init(void);

/*! \brief Allocates sleep queue entry. */
sleepq_t *sleepq_alloc(void);

/*! \brief Deallocates sleep queue entry. */
void sleepq_destroy(sleepq_t *sq);

/*! \brief Blocks the current thread until it is awakened from its sleep queue.
 *
 * \param wchan unique sleep queue identifier
 * \param waitpt caller associated with sleep action
 */
#define sleepq_wait(wchan, waitpt)                                             \
  ((void)_sleepq_wait(wchan, waitpt, SQ_NORMAL))

/*! \brief Same as \a sleepq_wait but allows the sleep to be aborted. */
#define sleepq_wait_abortable(wchan, waitpt)                                   \
  (_sleepq_wait(wchan, waitpt, SQ_ABORT))

/*! \brief Puts a thread to sleep until it's woken up or its sleep is aborted.
 *
 * If sleep is abortable other threads can wake up forcefully the thread with \a
 * sleepq_abort procedure.
 */
sq_wakeup_t _sleepq_wait(void *wchan, const void *waitpt, sq_wakeup_t sleep);

sq_wakeup_t sleepq_wait_timed(void *wchan, const void *waitpt,
                              systime_t timeout_ms);

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
