#ifndef _SYS_TURNSTILE_H_
#define _SYS_TURNSTILE_H_

#include <common.h>

typedef struct turnstile turnstile_t;

/*! \brief Initializes turnstiles.
 *
 * \warning To be called only from early kernel initialization! */
void turnstile_init(void);

/*! \brief Allocates turnstile entry. */
turnstile_t *turnstile_alloc(void);

/*! \brief Deallocates turnstile entry. */
void turnstile_destroy(turnstile_t *ts);

/* Re-sort list of blocked threads, on which td is,
 * after we changed td's priority.
 *
 * Propagate td's priority if td is now first on list and we increased
 * its priority, i.e. td->td_prio is higher than oldprio.
 *
 * \note This function was created for sched_set_prio.
 * \note Requires td_spin acquired. */
void turnstile_adjust(thread_t *td, prio_t oldprio);

/* Block the current thread on given waiting channel. This function will
 * context switch. */
void turnstile_wait(void *wchan, thread_t *owner, const void *waitpt);

/* Wakeup all threads waiting on given channel and adjust the priority of the
 * current thread appropriately. */
void turnstile_broadcast(void *wchan);

#endif /* !_SYS_TURNSTILE_H_ */
