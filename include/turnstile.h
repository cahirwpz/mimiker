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
 * Requires td_spin acquired. */
void turnstile_adjust(thread_t *td, prio_t oldprio);

/* Block the current thread on turnstile ts. This function will context
 * switch. */
void turnstile_wait(turnstile_t *ts, thread_t *owner, const void *waitpt);

/* Wakeup all threads on the blocked list and adjust the priority of the
 * current thread appropriately. */
void turnstile_broadcast(turnstile_t *ts);

/* Looks for turnstile associated with wchan in turnstile chains and returns
 * it or NULL if no turnstile is found in chains. */
turnstile_t *turnstile_lookup(void *wchan);

/* At first it runs turnstile_lookup and returns the result if it's not NULL.
 * If turnstile was not found in chains, it returns thread_self()'s
 * turnstile. */
turnstile_t *turnstile_acquire(void *wchan);

#endif /* !_SYS_TURNSTILE_H_ */
