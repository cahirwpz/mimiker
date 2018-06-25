#ifndef _SYS_SLEEPQ_H_
#define _SYS_SLEEPQ_H_

#include <common.h>
#include <queue.h>

typedef struct thread thread_t;
typedef struct sleepq sleepq_t;

// TODO comma before element?
typedef enum {
  SLEEPQ_WKP_REG, /* regular wakeup
                   * keep it as the first entry (because of flags below) */
  SLEEPQ_WKP_INT, /* thread interrupted */
  SLEEPQ_WKP_TIME /* time-out */
} slp_wakeup_t;

#define SLPF_OF_WKP(w) (1 << ((w)-1))

typedef uint8_t sleep_flags_t;
#define SLPF_INT SLPF_OF_WKP(SLEEPQ_WKP_INT)
#define SLPF_TIME SLPF_OF_WKP(SLEEPQ_WKP_TIME)

/*! \file sleepq.h */

/*! \brief Initializes sleep queues.
 *
 * \warning To be called only from early kernel initialization! */
void sleepq_init(void);

/*! \brief Allocates sleep queue entry. */
sleepq_t *sleepq_alloc(void);

/*! \brief Deallocates sleep queue entry. */
void sleepq_destroy(sleepq_t *sq);

// TODO maybe mention that it's equal to `sleepq_wait_abortable(..., ..., 0)` ?
/*! \brief Blocks the current thread until it is awakened from its sleep queue.
 *
 * \param wchan unique sleep queue identifier
 * \param waitpt caller associated with sleep action
 */
void sleepq_wait(void *wchan, const void *waitpt);

// TODO how to doxygen?
/* \brief Blocks the current thread until it is awakened from its sleep queue or
 *        an event specified in flags occurs.
 *
 * Other threads can interrupt this sleep with `sleepq_abort(thread, reason)`.
 *
 * `SLEEPQ_WKP_REG` is always accepted as a valid reason (because it's used
 * internally for regular wake-up).
 */
slp_wakeup_t sleepq_wait_abortable(void *wchan, const void *waitpt,
                                   sleep_flags_t flags);

/*! \brief Wakes up highest priority thread waiting on \a wchan.
 *
 * \param wchan unique sleep queue identifier
 */
bool sleepq_signal(void *wchan);

// TODO how to doxygen
// TODO should we allow trying to abort a non-sleeping thread? At this (some)
//      moment we do (and just return false)
// TODO maybe we should just forbid calling it with reason SLEEPQ_WKP_REG
//      (with some assert)
/* Interrupt some thread's sleep. Returns true on success (false if the thread
 * wasn't sleeping or doesn't accept the reason). The woken up thread will
 * receive the reason as a return value from `sleepq_wait_abortable`.
 *
 * `SLEEPQ_WKP_REG` is always accepted as a valid reason (because it's used
 * internally for regular wake-up) and you probably shouldn't pass it to this
 * function.
 */
bool sleepq_abort(thread_t *td, slp_wakeup_t reason);

/*! \brief Resume all threads sleeping on \a wchan.
 *
 * \param wchan unique sleep queue identifier
 */
bool sleepq_broadcast(void *wchan);

#endif /* !_SYS_SLEEPQ_H_ */
