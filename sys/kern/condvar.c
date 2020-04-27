#include <sys/condvar.h>
#include <sys/sleepq.h>
#include <sys/sched.h>
#include <sys/lock.h>

void cv_init(condvar_t *cv, const char *name) {
  cv->name = name;
  cv->waiters = 0;
}

void cv_wait(condvar_t *cv, lock_t m) {
  WITH_NO_PREEMPTION {
    cv->waiters++;
    lk_release(m);
    sleepq_wait(cv, __caller(0));
  }
  lk_acquire(m, __caller(0));
}

int cv_wait_timed(condvar_t *cv, lock_t m, systime_t timeout) {
  int status;
  WITH_NO_PREEMPTION {
    cv->waiters++;
    lk_release(m);
    status = sleepq_wait_timed(cv, __caller(0), timeout);
  }
  lk_acquire(m, __caller(0));
  return status;
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
