#include <condvar.h>
#include <sleepq.h>
#include <errno.h>
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

static int errno_of_sq_wakeup(sq_wakeup_t s) {
  if (s == SQ_NORMAL)
    return 0;
  if (s == SQ_TIMEOUT)
    return ETIMEDOUT;
  if (s == SQ_ABORT)
    return EINTR;

  panic("Unexpected value of sq_wakeup_t");
}

int cv_wait_intr(condvar_t *cv, mtx_t *mtx) {
  sq_wakeup_t status;
  WITH_NO_PREEMPTION {
    cv->waiters++;
    mtx_unlock(mtx);
    status = sleepq_wait_abortable(cv, __caller(0));
  }
  _mtx_lock(mtx, __caller(0));
  return errno_of_sq_wakeup(status);
}

int cv_wait_timed(condvar_t *cv, mtx_t *mtx, systime_t timeout_ms) {
  sq_wakeup_t status;
  WITH_NO_PREEMPTION {
    cv->waiters++;
    mtx_unlock(mtx);
    status = sleepq_wait_timed(cv, __caller(0), timeout_ms);
  }
  _mtx_lock(mtx, __caller(0));
  return errno_of_sq_wakeup(status);
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
