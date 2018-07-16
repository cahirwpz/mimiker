#ifndef _SYS_CONDVAR_H_
#define _SYS_CONDVAR_H_

#include <common.h>
// TODO we include it just for sq_wakeup_t. Maybe we should define cv_wakeup_t?
#include <sleepq.h>

typedef struct condvar {
  const char *name;
  int waiters;
} condvar_t;

typedef struct mtx mtx_t;

void cv_init(condvar_t *cv, const char *name);

/* TODO: Make conditional variable unusable after it has been destroyed. */
#define cv_destroy(m)

void cv_wait(condvar_t *cv, mtx_t *mtx);
sq_wakeup_t cv_wait_intr(condvar_t *cv, mtx_t *mtx);
sq_wakeup_t cv_wait_timed(condvar_t *cv, mtx_t *mtx, systime_t timeout_ms);
void cv_signal(condvar_t *cv);
void cv_broadcast(condvar_t *cv);

#endif /* !_SYS_CONDVAR_H_ */
