#ifndef _SYS_SLEEPQ_H_
#define _SYS_SLEEPQ_H_

#include <common.h>
#include <queue.h>
#include <callout.h>

typedef struct thread thread_t;
typedef struct sleepq sleepq_t;

/* sq_wakeup_t is a subset of sq_flags_t - with exactly one set */
// TODO sq_flags_t could represent a union of sq_wakeup_t - but is there a
//      point?
typedef uint8_t sq_flags_t;
typedef uint8_t sq_wakeup_t;

#define SQ_REGULAR (1 << 0)
#define SQ_INTERRUPT (1 << 1)
#define SQ_TIMED (1 << 2)

/*! \file sleepq.h */

/*! \brief Initializes sleep queues.
 *
 * \warning To be called only from early kernel initialization! */
void sleepq_init(void);

/*! \brief Allocates sleep queue entry. */
sleepq_t *sleepq_alloc(void);

/*! \brief Deallocates sleep queue entry. */
void sleepq_destroy(sleepq_t *sq);

// TODO how to doxygen? (macro)
/*! \brief Blocks the current thread until it is awakened from its sleep queue.
 *
 * \param wchan unique sleep queue identifier
 * \param waitpt caller associated with sleep action
 */
#define sleepq_wait(wchan, waitpt)                                             \
  ((void)sleepq_wait_abortable(wchan, waitpt, 0))

// TODO how to doxygen?
/* \brief Blocks the current thread until it is awakened from its sleep queue or
 *        an event specified in flags occurs.
 *
 * Other threads can cancel this sleep with `sleepq_abort(thread, reason)`.
 *
 * `SQ_REGULAR` is always accepted as a valid reason (because it's used
 * internally for regular wake-up).
 */
sq_wakeup_t sleepq_wait_abortable(void *wchan, const void *waitpt,
                                  sq_flags_t flags);

/*! \brief Wakes up highest priority thread waiting on \a wchan.
 *
 * \param wchan unique sleep queue identifier
 */
bool sleepq_signal(void *wchan);

// TODO how to doxygen
// TODO should we allow trying to abort a non-sleeping thread? At this (some)
//      moment we do (and just return false)
// TODO Maybe we should just forbid calling it with reason SQ_REGULAR
//      (with some assert) (see description below)
/* Cancel some thread's sleep. Returns true on success (false if the thread
 * wasn't sleeping or doesn't accept the reason). The woken up thread will
 * receive the reason as the return value from `sleepq_wait_abortable`.
 *
 * `SQ_REGULAR` is always accepted as a valid reason (because it's used
 * internally for regular wake-up) and you probably shouldn't pass it to this
 * function.
 */
bool sleepq_abort(thread_t *td, sq_wakeup_t reason);

/*! \brief Resume all threads sleeping on \a wchan.
 *
 * \param wchan unique sleep queue identifier
 */
bool sleepq_broadcast(void *wchan);

/* \brief Blocks the current thread until it is awakened from its sleep queue or
 *        the time runs out or a signal is received.
 *
 * Other threads can cancel this sleep with `sleepq_abort(thread, reason)` where
 * `reason` is one of `SQ_REGULAR`, `SQ_INTERRUPT`, `SQ_TIMED`.
 */
sq_wakeup_t sleepq_wait_timed(void *wchan, const void *waitpt,
                              systime_t timeout_ms);

/* \brief Prepare timed sleep for the current thread.
 *
 * The purpose of this function is combining sleep modes in unusual ways. For
 * usual use see `sleepq_wait_timed` or `sleepq_wait`.
 *
 * This will initialize callout in structure pointed by `wk` that will abort
 * current's thread sleep in `timeout_ms` miliseconds from now. Therefore the
 * actual sleep should be initiated soon to have accurate timing. To actually
 * go to sleep use `sleepq_wait_abortable`.
 *
 * After the sleep one should call `sq_finish_timed(wk)` before going
 * to sleep again (best to call it immediately after sleep).
 */
sq_flags_t sq_prepare_timed(callout_t *wk, systime_t timeout_ms, sq_flags_t f);

/* \brief Cleanup after timed sleep
 *
 * Call this function after sleep preceded by `sq_prepare_timed` before going
 * to sleep again (best to call it immediately after sleep).
 */
void sq_finish_timed(callout_t *wk);

#endif /* !_SYS_SLEEPQ_H_ */
