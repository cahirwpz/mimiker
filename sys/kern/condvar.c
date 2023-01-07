#include <sys/condvar.h>
#include <sys/mutex.h>
#include <sys/sleepq.h>

void cv_init(condvar_t *cv, const char *name) {
  cv->name = name;
  cv->waiters = 0;
}

void cv_wait(condvar_t *cv, mtx_t *m) {
  sleepq_lock(cv);
  cv->waiters++;
  mtx_unlock(m);
  sleepq_wait(cv, __caller(0));
  mtx_lock(m);
}

int cv_wait_timed(condvar_t *cv, mtx_t *m, systime_t timeout) {
  sleepq_lock(cv);
  cv->waiters++;
  mtx_unlock(m);
  int status = sleepq_wait_timed(cv, __caller(0), timeout);
  mtx_lock(m);
  return status;
}

void cv_signal(condvar_t *cv) {
  sleepq_lock(cv);
  if (!cv->waiters)
    goto end;
  cv->waiters--;
  sleepq_signal(cv);
end:
  sleepq_unlock(cv);
}

void cv_broadcast(condvar_t *cv) {
  sleepq_lock(cv);
  if (!cv->waiters)
    goto end;
  cv->waiters--;
  sleepq_broadcast(cv);
end:
  sleepq_unlock(cv);
}
