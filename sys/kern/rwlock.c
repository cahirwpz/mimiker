#include <sys/klog.h>
#include <sys/libkern.h>
#include <sys/rwlock.h>
#include <sys/thread.h>
#include <sys/sleepq.h>
#include <sys/sched.h>

void rw_init(rwlock_t *rw, const char *name, bool recursive) {
  rw->readers = 0;
  rw->writers_waiting = 0;
  rw->writer = NULL;
  rw->state = RW_UNLOCKED;
  rw->recursive = recursive;
  rw->name = name;
}

void rw_destroy(rwlock_t *rw) {
}

static bool is_owned(rwlock_t *rw) {
  return rw->writer == thread_self();
}

static bool is_locked(rwlock_t *rw) {
  return rw->state & RW_LOCKED;
}

static bool is_rlocked(rwlock_t *rw) {
  return rw->state == RW_RLOCKED;
}

static bool is_wlocked(rwlock_t *rw) {
  return rw->state == RW_WLOCKED;
}

void rw_enter(rwlock_t *rw, rwo_t who) {
  SCOPED_NO_PREEMPTION();
  if (who == RW_READER) {
    while (is_wlocked(rw) || rw->writers_waiting > 0)
      sleepq_wait(&rw->readers, __caller(0));
    rw->readers++;
    rw->state = RW_RLOCKED;
  } else if (who == RW_WRITER) {
    if (is_owned(rw)) {
      assert(rw->recursive);
      rw->recurse++;
    } else {
      rw->writers_waiting++;
      while (is_locked(rw))
        sleepq_wait(&rw->writer, __caller(0));
      rw->state = RW_WLOCKED;
      rw->writer = thread_self();
      rw->writers_waiting--;
    }
  }
}

void rw_leave(rwlock_t *rw) {
  SCOPED_NO_PREEMPTION();
  assert(is_locked(rw));
  if (is_rlocked(rw)) {
    rw->readers--;
    if (rw->readers == 0) {
      rw->state = RW_UNLOCKED;
      if (rw->writers_waiting > 0)
        sleepq_signal(&rw->writer);
    }
  } else {
    assert(is_owned(rw));
    if (rw->recurse > 0) {
      assert(rw->recursive);
      rw->recurse--;
    } else {
      rw->state = RW_UNLOCKED;
      rw->writer = NULL;
      if (rw->writers_waiting > 0)
        sleepq_signal(&rw->writer);
      else
        sleepq_broadcast(&rw->readers);
    }
  }
}

bool rw_try_upgrade(rwlock_t *rw) {
  SCOPED_NO_PREEMPTION();
  assert(is_locked(rw));
  if (is_rlocked(rw) && rw->readers == 1 && rw->writers_waiting == 0) {
    rw->state = RW_WLOCKED;
    rw->writer = thread_self();
    rw->recurse = 0;
    return true;
  }
  return false;
}

void rw_downgrade(rwlock_t *rw) {
  SCOPED_NO_PREEMPTION();
  assert(is_owned(rw) && rw->recurse == 0);
  rw->readers++;
  rw->state = RW_RLOCKED;
  rw->writer = NULL;
  sleepq_broadcast(&rw->readers);
}

void __rw_assert(rwlock_t *rw, rwa_t what, const char *file, unsigned line) {
  bool ok = (what == RW_UNLOCKED) ? !is_locked(rw) : (rw->state & what);
  if (!ok)
    klog("[%s:%d] rwlock (%p) has invalid state: expected %u, actual %u!", file,
         line, rw, what, rw->state);
}
