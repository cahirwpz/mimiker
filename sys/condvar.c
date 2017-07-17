#include <condvar.h>
#include <sleepq.h>
#include <sched.h>
#include <mutex.h>

void cv_init(condvar_t *cv, const char *name) {
  cv->name = name;
  cv->waiters = 0;
}

void cv_wait(condvar_t *cv, mtx_t *mtx) {
  WITH_NO_PREEMPTION {
    cv->waiters++;
    mtx_unlock(mtx);
    sleepq_wait(cv, __caller(0));
  }
  _mtx_lock(mtx, __caller(0));
}

void cv_signal(condvar_t *cv) {
  SCOPED_NO_PREEMPTION();
  if (cv->waiters > 0) {
    cv->waiters--;
    sleepq_signal(cv);
  }
}

void cv_broadcast(condvar_t *cv) {
  SCOPED_NO_PREEMPTION();
  if (cv->waiters > 0) {
    cv->waiters = 0;
    sleepq_broadcast(cv);
  }
}
