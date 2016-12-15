#ifndef _SYS_CONDVAR_H_
#define _SYS_CONDVAR_H_

typedef struct condvar {
  const char *name;
  int waiters;
} condvar_t;

typedef struct mtx mtx_t;

void cv_init(condvar_t *cv, const char *name);
void cv_destroy(condvar_t *cv);
void cv_wait(condvar_t *cv, mtx_t *mtx);
void cv_signal(condvar_t *cv);
void cv_broadcast(condvar_t *cv);

#endif /* !_SYS_CONDVAR_H_ */
