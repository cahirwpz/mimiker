#ifndef _SYS_MUTEX_H_
#define _SYS_MUTEX_H_

#include <turnstile.h>
#include <thread.h>

typedef struct {
  volatile uint32_t mtx_state; /* 0 if unowned or has address of the owner */
  turnstile_t turnstile;       /* FIFO Queue for blocked threads */
} mtx_t;

/* Initializes mutex. Note that EVERY mutex has to be initialized
 * before it is used. */
void mtx_init(mtx_t *);

/* Locks the mutex, if the mutex is already owned by someone,
 * then this blocks on turnstile, otherwise it takes the mutex.
 * At this moment mutexes cannot be recursive. */
void mtx_lock(mtx_t *);

/* Unlocks the mutex. If some thread blocked for the mutex, then it
 * wakes up the thread in FIFO manner. */
void mtx_unlock(mtx_t *);

/* Returns true iff the mutex is locked and we are the owner. */
bool mtx_owned(mtx_t *mtx);

#endif /* !_SYS_MUTEX_H_ */
