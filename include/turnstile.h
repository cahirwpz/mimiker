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

/* Block the current thread on turnstile ts. This function will context
 * switch. This function must be called with turnstile chain locked and will
 * return with it unlocked.
 *
 * Locks:
 *  needs:    ts_lock, tc_lock
 *  releases: ts_lock, tc_lock. */
void turnstile_wait(turnstile_t *ts, thread_t *owner, const void *waitpt);

/* Wakeup all threads on the blocked list and adjust the priority of the
 * current thread appropriately.
 *
 * Locks:
 *  needs:    ts_lock, tc_lock
 *  releases: ts_lock, tc_lock. */
void turnstile_broadcast(turnstile_t *ts);

/* Looks for turnstile associated with wchan in turnstile chains and returns
 * it or NULL if no turnstile is found in chains.
 *
 * Locks:
 *  acquires: tc_lock, ts_lock (ts_lock if turnstile is found). */
turnstile_t *turnstile_lookup(void *wchan);

/* At first it runs turnstile_lookup and returns the result if it's not NULL.
 * If turnstile was not found in chains, it returns thread_self()'s turnstile.
 *
 * Locks:
 *  acquires: tc_lock, ts_lock. */
turnstile_t *turnstile_acquire(void *wchan);

/* Locks turnstile chain associated with wchan and returns pointer
 * to this chain.
 *
 * Locks:
 *  acquires: tc_lock. */
turnstile_chain_t *turnstile_chain_lock(void *wchan);

/* Unlocks turnstile chain associated with wchan.
 *
 * Locks:
 *  releases: tc_lock. */
void turnstile_chain_unlock(void *wchan);

void turnstile_broadcast_wchan(void *wchan);

void turnstile_wait_wchan(void *wchan, thread_t *owner, const void *waitpt);

#endif /* !_SYS_TURNSTILE_H_ */
