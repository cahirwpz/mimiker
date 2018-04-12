#ifndef _SYS_TURNSTILE_H_
#define _SYS_TURNSTILE_H_

#include <common.h>

typedef struct turnstile turnstile_t;

void turnstile_init(void);

turnstile_t *turnstile_alloc(void);

void turnstile_destroy(turnstile_t *ts);

void turnstile_adjust(thread_t *td, td_prio_t oldprio);

void turnstile_wait(turnstile_t *ts, thread_t *owner);

void turnstile_broadcast(turnstile_t *ts);

void turnstile_unpend(turnstile_t *ts);

void turnstile_chain_lock(void *wchan);

void turnstile_chain_unlock(void *wchan);

turnstile_t *turnstile_trywait(void *wchan);

void turnstile_cancel(turnstile_t *ts);

turnstile_t *turnstile_lookup(void *wchan);

#endif /* !_SYS_TURNSTILE_H_ */
