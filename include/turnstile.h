#ifndef _SYS_TURNSTILE_H_
#define _SYS_TURNSTILE_H_

#include <common.h>

typedef struct turnstile turnstile_t;
typedef struct turnstile_chain turnstile_chain_t;

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
 * its priority, i.e. td->td_prio > oldprio.
 *
 * \note This function was created for sched_set_prio.
 *
 * Locks:
 *  needs: td_spin. */
void turnstile_adjust(thread_t *td, prio_t oldprio);

// TODO what does it do?
void turnstile_broadcast(void *wchan);

// TODO what does it do?
void turnstile_wait(void *wchan, thread_t *owner, const void *waitpt);

#endif /* !_SYS_TURNSTILE_H_ */
