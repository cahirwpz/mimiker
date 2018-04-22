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

/* \brief Adjusts thread on list of threads blocked waiting for some lock.
 *
 * \note This function is called from scheduler's \a sched_set_prio when
 * it changes priority of thread \a td.
*/
void turnstile_adjust(thread_t *td, td_prio_t oldprio);

/*** actual stuff ***/

// Block on turnstile ts.
// Needed locks: ts->ts_lock, tc->tc_lock
// Locks kept: none
void turnstile_wait(turnstile_t *ts, thread_t *owner);

// Needed locks: ts->ts_lock, tc->tc_lock
// Kept locks: all
void turnstile_broadcast(turnstile_t *ts);

// Needed locks: ts->ts_lock, tc->tc_lock
// Kept locks: tc->tc_lock
void turnstile_unpend(turnstile_t *ts, void *wchan);

/* Looks for turnstile associated with wchan in turnstile chains and returns
 * it or NULL if no turnstile is found in chains.
 *
 * Acquires tc_lock, ts_lock (ts_lock if turnstile is found). */
turnstile_t *turnstile_lookup(void *wchan);

/* At first it runs turnstile_lookup and returns the result if it's not NULL.
 * If turnstile was not found in chains, it returns thread_self()'s turnstile.
 *
 * Acquires tc_lock, ts_lock. */
turnstile_t *turnstile_acquire(void *wchan);

/* Locks turnstile chain associated with wchan and returns pointer
 * to this chain.
 *
 * Acquires tc_lock. */
turnstile_chain_t *turnstile_chain_lock(void *wchan);

/* Unlocks turnstile chain associated with wchan.
 *
 * Releases tc_lock. */
void turnstile_chain_unlock(void *wchan);

#endif /* !_SYS_TURNSTILE_H_ */
