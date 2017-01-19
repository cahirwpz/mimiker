#include <rwlock.h>
#include <condvar.h>
#include <mutex.h>
#include <thread.h>
#include <stdc.h>

/*
32   28           16                 0
 |  F |            |                 |
 |  L |   WRITERS  |    READERS OR   |
 |  A |   WAITING  |  WRITER ENTERS  |
 |  G |            |     COUNTER     |
 |  S |            |                 |
 |____|____________|_________________|
*/
#define F_MASK 0xF0000000 //FLAGS MASK
#define S_MASK 0xC0000000 //STATE MASK
#define WW_MASK 0x0FFF0000 //WRITERS WAITING MASK
#define CNT_MASK 0x0000FFFF //COUNTER MASK

#define SET_UNLOCKED_STATE(info) ((info) = ((info) & ~S_MASK) | (1 << (31-RW_UNLOCKED)))
#define SET_RLOCKED_STATE(info) ((info) = ((info) & ~S_MASK) | (1 << (31-RW_READER)) | (1 << (31-RW_LOCKED)))
#define SET_WLOCKED_STATE(info) ((info) = ((info) & ~S_MASK) | (1 << (31-RW_WLOCKED)) | (1 << (31-RW_LOCKED)))
#define IS_FLAG_SET(info, flag) ((info & F_MASK) & (1 << (31-(flag))))

#define INC_WRITERS_WAITING(info) ((info) = (((((info) & WW_MASK >> 16)+1) << 16) & WW_MASK) | ((info) & (~WW_MASK)))
#define DEC_WRITERS_WAITING(info) ((info) = (((((info) & WW_MASK >> 16)-1) << 16) & WW_MASK) | ((info) & (~WW_MASK)))
#define GET_WRITERS_WAITING(info) (((info) & WW_MASK) >> 16)

#define INC_COUNTER(info) ((info) = ((((info) & CNT_MASK)+1) & CNT_MASK) | ((info) & (~CNT_MASK)))
#define DEC_COUNTER(info) ((info) = ((((info) & CNT_MASK)-1) & CNT_MASK) | ((info) & (~CNT_MASK)))
#define GET_COUNTER(info) ((info) & CNT_MASK)

struct rwlock {
  int info;
  condvar_t condvar;
  mtx_t mutex;
  const char* name;
  thread_t* owner;
};

void rw_init(rwlock_t *rw, const char *name, bool recurse) {
  rw->info = ((!!recurse) << (31-RW_RECURSE));
  SET_UNLOCKED_STATE(rw->info);
  rw->name = name;
  rw->owner = NULL;
  mtx_init(&rw->mutex);
  cv_init(&rw->condvar, name);
}

void rw_destroy(rwlock_t *rw) {
}

void rw_enter(rwlock_t *rw, rwo_t who) {
  mtx_lock(&rw->mutex);
  if(who == RW_READER) {
    while(IS_FLAG_SET(rw->info, RW_WLOCKED) || GET_WRITERS_WAITING(rw->info) > 0)
      cv_wait(&rw->condvar, &rw->mutex);
    INC_COUNTER(rw->info);
    SET_RLOCKED_STATE(rw->info);
  }
  else if(who == RW_WRITER) {
    INC_WRITERS_WAITING(rw->info);
    assert(!(IS_FLAG_SET(rw->info, RW_WLOCKED) && !IS_FLAG_SET(rw->info, RW_RECURSE) && rw->owner == thread_self()));
    while((IS_FLAG_SET(rw->info, RW_RLOCKED)) ||
          (IS_FLAG_SET(rw->info, RW_WLOCKED) && !IS_FLAG_SET(rw->info, RW_RECURSE)) ||
          (IS_FLAG_SET(rw->info, RW_WLOCKED) && IS_FLAG_SET(rw->info, RW_RECURSE) && rw->owner != thread_self()))
      cv_wait(&rw->condvar, &rw->mutex);
    INC_COUNTER(rw->info);
    SET_WLOCKED_STATE(rw->info);
    rw->owner = thread_self();
    DEC_WRITERS_WAITING(rw->info);
  }
  mtx_unlock(&rw->mutex);
}

void rw_leave(rwlock_t *rw) {
  mtx_lock(&rw->mutex);
  assert(IS_FLAG_SET(rw->info, RW_LOCKED));
  DEC_COUNTER(rw->info);
  if(GET_COUNTER(rw->info) == 0) {
    cv_broadcast(&rw->condvar);
    SET_UNLOCKED_STATE(rw->info);
    rw->owner = NULL;
  }
  mtx_unlock(&rw->mutex);
}

bool rw_try_upgrade(rwlock_t *rw) {
  mtx_lock(&rw->mutex);
  assert(IS_FLAG_SET(rw->info, RW_RLOCKED));
  bool result = false;
  if(GET_COUNTER(rw->info) == 1) {
    SET_WLOCKED_STATE(rw->info);
    rw->owner = thread_self();
    result = true;
  }
  mtx_unlock(&rw->mutex);
  return result;
}

void rw_downgrade(rwlock_t *rw) {
  mtx_lock(&rw->mutex);
  assert(IS_FLAG_SET(rw->info, RW_WLOCKED) && rw->owner == thread_self());
  SET_RLOCKED_STATE(rw->info);
  cv_broadcast(&rw->condvar);
  mtx_unlock(&rw->mutex);
}

void __rw_assert(rwlock_t *rw, rwf_t flag, const char *file, unsigned line) {
  if(IS_FLAG_SET(rw->info, flag))
    panic("rwlock %p: %u flag is not set. File:%s line:%u", rw, flag, file, line);
}
