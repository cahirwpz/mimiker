#ifndef _SYS_TURNSTILE_H_
#define _SYS_TURNSTILE_H_

#include <common.h>

typedef struct turnstile turnstile_t;

/*** technical stuff ***/

void turnstile_init(void);

turnstile_t *turnstile_alloc(void);

void turnstile_destroy(turnstile_t *ts);

/*** scheduler stuff ***/

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

turnstile_t *turnstile_trywait(void *wchan);

void turnstile_cancel(turnstile_t *ts);

turnstile_t *turnstile_lookup(void *wchan);

#endif /* !_SYS_TURNSTILE_H_ */
