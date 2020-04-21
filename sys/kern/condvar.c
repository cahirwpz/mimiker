#include <sys/condvar.h>
#include <sys/sleepq.h>
#include <sys/sched.h>
#include <sys/mutex.h>

#define spin_lock_p(m) lock_attrs_spinning(*(m).attrs)

static void mutex_lock(cv_lock_t m, const void *waitpt) {
  if (spin_lock_p(m))
    _spin_lock(m.spin, waitpt);
  else
    _mtx_lock(m.mtx, waitpt);
}

static void mutex_unlock(cv_lock_t m) {
  if (spin_lock_p(m))
    spin_unlock(m.spin);
  else
    mtx_unlock(m.mtx);
}

void cv_init(condvar_t *cv, const char *name) {
  cv->name = name;
  cv->waiters = 0;
}

void cv_wait(condvar_t *cv, cv_lock_t m) {
  WITH_NO_PREEMPTION {
    cv->waiters++;
    mutex_unlock(m);
    sleepq_wait(cv, __caller(0));
  }
  mutex_lock(m, __caller(0));
}

int cv_wait_timed(condvar_t *cv, cv_lock_t m, systime_t timeout) {
  int status;
  WITH_NO_PREEMPTION {
    cv->waiters++;
    mutex_unlock(m);
    status = sleepq_wait_timed(cv, __caller(0), timeout);
  }
  mutex_lock(m, __caller(0));
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
