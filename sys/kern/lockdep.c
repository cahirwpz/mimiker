#include <sys/lockdep.h>
#include <sys/spinlock.h>
#include <sys/thread.h>
#include <sys/mimiker.h>

typedef TAILQ_HEAD(, lock_class) lock_class_list_t;

static spin_t main_lock = SPIN_INITIALIZER(0);

#define CLASSHASH_SIZE 64
#define CLASSHASH(key) ((unsigned int)(key) % CLASSHASH_SIZE)
#define CLASS_HASH_CHAIN(key) (&lock_hashtbl[CLASSHASH(key)])

static lock_class_list_t lock_hashtbl[CLASSHASH_SIZE];

#define MAX_CLASSES 64
static lock_class_t lock_classes[MAX_CLASSES];
static int class_cnt = 0;

#define MAX_LINKS 256
static lock_class_link_t lock_links[MAX_LINKS];
static int link_cnt = 0;

static int lockdep_initialized = 0;

static inline void lockdep_lock(void) {
  spin_lock(&main_lock);
}

static inline void lockdep_unlock(void) {
  spin_unlock(&main_lock);
}

static void init_structures(void) {
  if (lockdep_initialized)
    return;
  lockdep_initialized = 1;
  for (int i = 0; i < CLASSHASH_SIZE; i++) {
    TAILQ_INIT(&lock_hashtbl[i]);
  }
}

static lock_class_t *look_up_lock_class(lock_class_mapping_t *lock) {
  lock_class_key_t *key = lock->key;
  lock_class_t *class;

  TAILQ_FOREACH (class, CLASS_HASH_CHAIN(key), hash_entry) {
    if (class->key == key)
      return class;
  }

  return NULL;
}

static lock_class_t *alloc_class(void) {
  if (class_cnt >= MAX_CLASSES)
    panic("linkdep: no more classes");

  return &lock_classes[class_cnt++];
}

static lock_class_t *register_lock_class(lock_class_mapping_t *lock) {
  lock_class_t *class;

  /* If the lock doesn't have a key then it is statically allocated. In that
   * case use its address as the key. */
  if (lock->key == NULL)
    lock->key = (void *)lock;

  class = look_up_lock_class(lock);
  if (!class) {
    init_structures();
    class = alloc_class();
    class->key = lock->key;
    class->name = lock->name;
    TAILQ_INIT(&class->locked_after);
    TAILQ_INSERT_HEAD(CLASS_HASH_CHAIN(lock->key), class, hash_entry);

    lock->lock_class = class;
  }

  return class;
}

static lock_class_link_t *alloc_link(void) {
  if (link_cnt >= MAX_LINKS)
    panic("linkdep: no more links");

  return &lock_links[link_cnt++];
}

static void add_prev_link(void) {
  int depth = thread_self()->td_lock_depth;
  held_lock_t *hprev, *hcur;
  lock_class_link_t *link;

  if (depth < 2)
    return;

  hcur = &thread_self()->td_held_locks[depth - 1];
  hprev = &thread_self()->td_held_locks[depth - 2];

  TAILQ_FOREACH (link, &(hprev->lock_class->locked_after), entry) {
    if (hcur->lock_class == link->to)
      return;
  }

  link = alloc_link();
  link->from = hprev->lock_class;
  link->to = hcur->lock_class;

  TAILQ_INSERT_HEAD(&(hprev->lock_class->locked_after), link, entry);
}

void lock_acquire(lock_class_mapping_t *lock) {
  lock_class_t *class;
  held_lock_t *hlock;

  lockdep_lock();

  if (thread_self()->td_lock_depth >= MAX_LOCK_DEPTH)
    panic("lockdep: max lock depth reached");

  class = lock->lock_class;
  if (class == NULL)
    class = register_lock_class(lock);

  hlock = &thread_self()->td_held_locks[thread_self()->td_lock_depth];
  hlock->lock_class = class;

  thread_self()->td_lock_depth++;

  add_prev_link();

  lockdep_unlock();
}

void lock_release(void) {
  lockdep_lock();

  thread_self()->td_lock_depth--;

  if (thread_self()->td_lock_depth < 0)
    panic("lockdep: depth below 0");

  lockdep_unlock();
}

void lockdep_print_graph(void) {
  lockdep_lock();
  lock_class_link_t *link;

  kprintf("%d\n", class_cnt);
  for (int i = 0; i < class_cnt; i++) {
    kprintf("%d %s ", i, lock_classes[i].name);
    TAILQ_FOREACH (link, &(lock_classes[i].locked_after), entry) {
      int class_idx = link->to - lock_classes;
      kprintf("%d ", class_idx);
    }
    kprintf("\n");
  }

  lockdep_unlock();
}