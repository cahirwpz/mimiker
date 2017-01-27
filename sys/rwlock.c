#include <rwlock.h>
#include <thread.h>
#include <stdc.h>
#include <sync.h>

struct rwlock {
  union {
    int readers;
    int writer_enters;
  };
  int writers_waiting;
  thread_t *owner;
  rwa_t state;
  bool recurse;
  const char *name;
};

void rw_init(rwlock_t *rw, const char *name, bool recurse) {
  rw->readers = 0;
  rw->writers_waiting = 0;
  rw->owner = NULL;
  rw->state = RW_UNLOCKED;
  rw->recurse = recurse;
  rw->name = name;
}

void rw_destroy(rwlock_t *rw) {
}

void rw_enter(rwlock_t *rw, rwo_t who) {
  critical_enter();
  if (who == RW_READER) {
    while (rw->state == RW_WLOCKED || rw->writers_waiting > 0)
      sleepq_wait(&rw->owner, NULL);
    rw->readers++;
    rw->state = RW_RLOCKED;
  } else if (who == RW_WRITER) {
    assert(
      !(rw->state == RW_WLOCKED && !rw->recurse && rw->owner == thread_self()));
    rw->writers_waiting++;
    while (
      (rw->state == RW_RLOCKED) || (rw->state == RW_WLOCKED && !rw->recurse) ||
      (rw->state == RW_WLOCKED && rw->recurse && rw->owner != thread_self()))
      sleepq_wait(&rw->owner, NULL);
    rw->writer_enters++;
    rw->state = RW_WLOCKED;
    rw->owner = thread_self();
    rw->writers_waiting--;
  }
  critical_leave();
}

void rw_leave(rwlock_t *rw) {
  critical_enter();
  assert(rw->state & RW_LOCKED);
  rw->readers--;
  if (rw->readers == 0) {
    sleepq_broadcast(&rw->owner);
    rw->state = RW_UNLOCKED;
    rw->owner = NULL;
  }
  critical_leave();
}

bool rw_try_upgrade(rwlock_t *rw) {
  critical_enter();
  assert(rw->state == RW_RLOCKED);
  bool result = false;
  if (rw->readers == 1) {
    rw->state = RW_WLOCKED;
    rw->owner = thread_self();
    result = true;
  }
  critical_leave();
  return result;
}

void rw_downgrade(rwlock_t *rw) {
  critical_enter();
  assert(rw->state == RW_WLOCKED && rw->owner == thread_self() &&
         rw->writer_enters == 1);
  rw->state = RW_RLOCKED;
  sleepq_broadcast(&rw->owner);
  critical_leave();
}

void __rw_assert(rwlock_t *rw, rwa_t what, const char *file, unsigned line) {
  if (!(rw->state == RW_UNLOCKED && what == RW_UNLOCKED) || !(rw->state & what))
    kprintf("[%s:%d] rwlock (%p) has invalid state: expected %u, actual %u!\n",
            file, line, rw, what, rw->state);
}
