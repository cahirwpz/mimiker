#ifndef _SYS_LOCKDEP_H_
#define _SYS_LOCKDEP_H_

#include <sys/queue.h>
#include <sys/types.h>

#define LOCKDEP_MAX_HELD_LOCKS 16

typedef struct lock_class lock_class_t;

typedef uintptr_t lock_class_key_t;

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