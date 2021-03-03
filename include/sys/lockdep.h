#ifndef _SYS_LOCKDEP_H_
#define _SYS_LOCKDEP_H_

#include <sys/queue.h>

#define MAX_LOCK_DEPTH 16

typedef struct lock_class lock_class_t;

/* lock_class_link represents an edge in the graph of lock depedency. */
typedef struct lock_class_link {
  TAILQ_ENTRY(lock_class_link) entry;
  lock_class_t *from, *to;
} lock_class_link_t;

typedef int lock_class_key_t;

typedef struct lock_class {
  TAILQ_ENTRY(lock_class) hash_entry;
  lock_class_key_t *key;
  const char *name;
  TAILQ_HEAD(, lock_class_link) locked_after;

  int bfs_visited;
} lock_class_t;

/* A struct which is part of every mutex object. */
typedef struct lock_class_mapping {
  lock_class_key_t *key;
  const char *name;
  lock_class_t *lock_class;
} lock_class_mapping_t;

/* Informations about the lock held by a thread. */
typedef struct held_lock {
  lock_class_t *lock_class;
} held_lock_t;

void lockdep_init(void);

void lockdep_acquire(lock_class_mapping_t *lock);
void lockdep_release(void);

#endif /* !_SYS_LOCKDEP_H_ */