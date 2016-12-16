#include <condvar.h>
#include <sleepq.h>
#include <sync.h>
#include <mutex.h>

void cv_init(condvar_t *cv, const char *name) {
  cv->name = name;
  cv->waiters = 0;
}

void cv_wait(condvar_t *cv, mtx_t *mtx) {
  critical_enter();
  cv->waiters++;
  mtx_unlock(mtx);
  sleepq_wait(cv, cv->name);
  critical_leave();
  mtx_lock(mtx);
}

void cv_signal(condvar_t *cv) {
  critical_enter();
  if (cv->waiters > 0) {
    cv->waiters--;
    sleepq_signal(cv);
  }
  critical_leave();
}

void cv_broadcast(condvar_t *cv) {
  critical_enter();
  if (cv->waiters > 0) {
    cv->waiters = 0;
    sleepq_broadcast(cv);
  }
  critical_leave();
}
