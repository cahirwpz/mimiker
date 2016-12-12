#ifndef __TURNSTILE_H__
#define __TURNSTILE_H__
#include <queue.h>

typedef struct { TAILQ_HEAD(, thread) td_queue; } turnstile_t;

/* Initialize turnstile. */
void turnstile_init(turnstile_t *);

/* This puts currently running thread on the turnstile,
 * puts it in waiting state and yields. While waiting on turnstile
 * thread cannot be run. This function has to be called with critical section */
void turnstile_wait(turnstile_t *);

/* Removes first thread from the turnstile and puts it into run queue.
 * This function is has to be called with critical section. */
void turnstile_signal(turnstile_t *);
#endif /* __TURNSTILE_H__ */
