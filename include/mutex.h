#ifndef __MUTEX_H__
#define __MUTEX_H__

#include <common.h>
#include <mips/cpu.h>
#include <mips/mips.h>
#include <turnstile.h>

typedef volatile uintptr_t mtx_t;

#define MTX_INITIALIZER 0

#define mtx_lock(m) __extension__({ m = _mips_intdisable(); })

#define mtx_unlock(m) __extension__({ _mips_intrestore(m); })

#define MTX_UNOWNED 0

typedef struct {
  volatile uint32_t mtx_state; /* 0 if unowned or has address of the owner */
  turnstile_t turnstile;       /* FIFO Queue for blocked threads */
} mtx_sleep_t;

/* Initializes mutex. Note that EVERY mutex has to be initialized
 * before it is used. */
void mtx_sleep_init(mtx_sleep_t *);

/* Locks the mutex, if the mutex is already owned by someone,
 * then this blocks on turnstile, otherwise it takes the mutex.
 * At this moment mutexes cannot be recursive. */
void mtx_sleep_lock(mtx_sleep_t *);

/* Unlocks the mutex. If some thread blocked for the mutex, then it
 * wakes up the thread in FIFO manner. */
void mtx_sleep_unlock(mtx_sleep_t *);

#endif /* __MUTEX_H__ */
