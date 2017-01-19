#include <rwlock.h>
#include <condvar.h>
#include <mutex.h>
#include <thread.h>
#include <stdc.h>

struct rwlock {
  union {
    int readers;
    int writer_enters;
  };
  int writers_waiting;
  thread_t* owner;
  rwa_t state;
  bool recurse;
  const char* name;
  condvar_t condvar;
  mtx_t mutex;
};

void rw_init(rwlock_t *rw, const char *name, bool recurse) {
  rw->readers = 0;
  rw->writers_waiting = 0;
  rw->owner = NULL;
  rw->state = RW_UNLOCKED;
  rw->recurse = recurse;
  rw->name = name;
  mtx_init(&rw->mutex);
  cv_init(&rw->condvar, name);
}

void rw_destroy(rwlock_t *rw) {
}

void rw_enter(rwlock_t *rw, rwo_t who) {
  mtx_lock(&rw->mutex);
  if(who == RW_READER) {
    while(rw->state == RW_WLOCKED || rw->writers_waiting > 0)
      cv_wait(&rw->condvar, &rw->mutex);
    rw->readers++;
    rw->state = RW_RLOCKED;
  }
  else if(who == RW_WRITER) {
    assert(!(rw->state == RW_WLOCKED && !rw->recurse && rw->owner == thread_self()));
    rw->writers_waiting++;
    while((rw->state == RW_RLOCKED) ||
          (rw->state == RW_WLOCKED && !rw->recurse) ||
          (rw->state == RW_WLOCKED && rw->recurse && rw->owner != thread_self()))
      cv_wait(&rw->condvar, &rw->mutex);
    rw->writer_enters++;
    rw->state = RW_WLOCKED;
    rw->owner = thread_self();
    rw->writers_waiting--;
  }
  mtx_unlock(&rw->mutex);
}

void rw_leave(rwlock_t *rw) {
  mtx_lock(&rw->mutex);
  assert(rw->state & RW_LOCKED);
  rw->readers--;
  if(rw->readers == 0) {
    cv_broadcast(&rw->condvar);
    rw->state = RW_UNLOCKED;
    rw->owner = NULL;
  }
  mtx_unlock(&rw->mutex);
}

bool rw_try_upgrade(rwlock_t *rw) {
  mtx_lock(&rw->mutex);
  assert(rw->state == RW_RLOCKED);
  bool result = false;
  if(rw->readers == 1) {
    rw->state = RW_WLOCKED;
    rw->owner = thread_self();
    result = true;
  }
  mtx_unlock(&rw->mutex);
  return result;
}

void rw_downgrade(rwlock_t *rw) {
  mtx_lock(&rw->mutex);
  assert(rw->state == RW_WLOCKED && rw->owner == thread_self() && rw->writer_enters == 1);
  rw->state = RW_RLOCKED;
  cv_broadcast(&rw->condvar);
  mtx_unlock(&rw->mutex);
}

void __rw_assert(rwlock_t *rw, rwa_t what, const char *file, unsigned line) {
  if(!(rw->state == RW_UNLOCKED && what == RW_UNLOCKED) || !(rw->state & what))
    panic("rwlock %p: has invalid state, expected:%u, actual:%u. File:%s line:%u", rw, what, rw->state, file, line);
}
