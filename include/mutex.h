#ifndef _SYS_MUTEX_H_
#define _SYS_MUTEX_H_

#include <stdbool.h>
#include <common.h>

typedef struct thread thread_t;

#define MTX_DEF 0
#define MTX_RECURSE 1

typedef struct mtx {
  volatile thread_t *m_owner; /* stores address of the owner */
  volatile unsigned m_count;  /* Counter for recursive mutexes */
  unsigned m_type;            /* Normal or recursive mutex */
} mtx_t;

/* Initializes mutex. Note that EVERY mutex has to be initialized
 * before it is used. */
void mtx_init(mtx_t *m, unsigned type);

/* Returns true iff the mutex is locked and we are the owner. */
bool mtx_owned(mtx_t *mtx);

/* Locks the mutex, if the mutex is already owned by someone,
 * then this blocks on sleepqueue, otherwise it takes the mutex. */
void mtx_lock(mtx_t *m);

/* Unlocks the mutex. If some thread blocked for the mutex,
 * then it wakes up the thread in FIFO manner. */
void mtx_unlock(mtx_t *m);

/* Use mtx_lock_guard to lock a mutex and have it automatically unlock when
   leaving current scope. */
DEFINE_CLEANUP_FUNCTION(mtx_t *, mtx_unlock);
#define mtx_scoped_lock(pmtx)                                                  \
  mtx_t *mtx_scoped_lock_##__LINE__ cleanup(mtx_unlock) = pmtx;                \
  mtx_lock(pmtx);

#endif /* !_SYS_MUTEX_H_ */
