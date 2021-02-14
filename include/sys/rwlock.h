#ifndef _SYS_RWLOCK_H_
#define _SYS_RWLOCK_H_

#include <stdbool.h>
#include <sys/cdefs.h>
#include <sys/mimiker.h>

typedef enum { RW_READER, RW_WRITER } rwo_t;

typedef enum {
  RW_UNLOCKED = 0,
  RW_RLOCKED = 1,
  RW_WLOCKED = 2,
  RW_LOCKED = 3
} rwa_t;

typedef struct thread thread_t;

typedef struct rwlock {
  /* private */
  union {
    int readers;
    int recurse;
  };
  int writers_waiting;
  thread_t *writer; /* sleepq address for writers */
  rwa_t state;
  bool recursive;
  /* public, read-only */
  const char *name;
} rwlock_t;

void rw_init(rwlock_t *rw, const char *name, bool recursive);
void rw_destroy(rwlock_t *rw);
void rw_enter(rwlock_t *rw, rwo_t who);
void rw_leave(rwlock_t *rw);
bool rw_try_upgrade(rwlock_t *rw);
void rw_downgrade(rwlock_t *rw);

DEFINE_CLEANUP_FUNCTION(rwlock_t *, rw_leave);

#define SCOPED_RW_ENTER(rwlock_p, who)                                         \
  SCOPED_STMT(rwlock_t, rw_enter, CLEANUP_FUNCTION(rw_leave), rwlock_p, who)

#define WITH_RW_LOCK(rwlock_p, who)                                            \
  WITH_STMT(rwlock_t, rw_enter, CLEANUP_FUNCTION(rw_leave), rwlock_p, who)

#define rw_assert(rw, what) __rw_assert((rw), (what), __FILE__, __LINE__)
void __rw_assert(rwlock_t *rw, rwa_t what, const char *file, unsigned line);

#endif /* !_SYS_RWLOCK_H_ */
