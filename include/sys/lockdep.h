#ifndef _SYS_LOCKDEP_H_
#define _SYS_LOCKDEP_H_

#include <sys/queue.h>
#include <sys/types.h>

/*
 * Lock depedency validator builds the graph between locks where edge denotes
 * that lock X was acquired while holding lock Y. This allows for detecting a
 * situation when there is no ordering (i.e. X is sometimes locked before Y and
 * vice-versa) between some pair of locks.
 *
 * The basic object the validator operates upon is a 'class' of locks. A class
 * of locks is a group of locks that are logically the same with respect to
 * locking rules, even if the locks may have multiple instantiations. For
 * example a lock proc_t::p_lock is one class, while each proc_t has its own
 * instantiation of that lock class.
 *
 * Every lock class is identified by its key. For statically allocated lock
 * instances the key equals to the pointer of that lock. Dynamically allocated
 * ones get the key depending on where they were initialized.
 *
 * Lock depedency validator uses pre-allocated global memory for its internal
 * structures.
 *
 * To enable, compile the kernel with LOCKDEP=1 flag.
 *
 * Every lock acquisition is instrumented in the way that we try to add an edge
 * between corresponding classes. If it does not exist we add it to the graph
 * and then we run the bfs to search for cycles. If the lock order is violated
 * the kernel will panic.
 */

#define LOCKDEP_MAX_HELD_LOCKS 16

typedef struct lock_class lock_class_t;
typedef uintptr_t lock_class_key_t;

/* A struct which is part of every lock object. */
typedef struct lock_class_mapping {
  lock_class_key_t *key;
  const char *name;
  lock_class_t *lock_class;
} lock_class_mapping_t;

#define LOCKDEP_MAPPING_INITIALIZER(lockname)                                  \
  { .key = NULL, .name = #lockname, .lock_class = NULL }

#if LOCKDEP
void lockdep_init(void);
#else
#define lockdep_init() __nothing
#endif

void lockdep_acquire(lock_class_mapping_t *lock);
void lockdep_release(lock_class_mapping_t *lock);

#endif /* !_SYS_LOCKDEP_H_ */
