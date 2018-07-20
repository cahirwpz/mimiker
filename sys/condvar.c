#include <condvar.h>
#include <sleepq.h>
#include <errno.h>
#include <sched.h>
#include <mutex.h>

void cv_init(condvar_t *cv, const char *name) {
  cv->name = name;
  cv->waiters = 0;
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

static int _cv_wait(condvar_t *cv, mtx_t *mtx, sq_wakeup_t wkp, systime_t tm) {
  sq_wakeup_t status;
  WITH_NO_PREEMPTION {
    cv->waiters++;
    mtx_unlock(mtx);
    status = _sleepq_wait(cv, __caller(0), wkp, tm);
  }
  _mtx_lock(mtx, __caller(0));
  return errno_of_sq_wakeup(status);
}

void cv_wait(condvar_t *cv, mtx_t *mtx) {
  _cv_wait(cv, mtx, SQ_NORMAL, 0);
}

int cv_wait_intr(condvar_t *cv, mtx_t *mtx) {
  return _cv_wait(cv, mtx, SQ_ABORT, 0);
}

int cv_wait_timed(condvar_t *cv, mtx_t *mtx, systime_t timeout) {
  return _cv_wait(cv, mtx, SQ_TIMEOUT, timeout);
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
