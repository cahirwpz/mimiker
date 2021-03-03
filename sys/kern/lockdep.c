#if LOCKDEP

#include <sys/lockdep.h>
#include <sys/spinlock.h>
#include <sys/thread.h>
#include <sys/mimiker.h>
#include <sys/klog.h>

typedef TAILQ_HEAD(, lock_class) lock_class_list_t;

static spin_t main_lock = SPIN_INITIALIZER(0);

#define CLASSHASH_SIZE 64
#define CLASSHASH(key) ((uintptr_t)(key) % CLASSHASH_SIZE)
#define CLASS_HASH_CHAIN(key) (&lock_hashtbl[CLASSHASH(key)])

static lock_class_list_t lock_hashtbl[CLASSHASH_SIZE];

#define MAX_CLASSES 64
static lock_class_t lock_classes[MAX_CLASSES];
static int class_cnt = 0;

#define MAX_LINKS 256
static lock_class_link_t lock_links[MAX_LINKS];
static int link_cnt = 0;

#define BFS_QUEUE_SIZE 32

typedef struct {
  lock_class_link_t *items[BFS_QUEUE_SIZE];
  size_t head, tail;
} bfs_queue_t;

static bfs_queue_t bfs_queue;

static inline void bfs_q_init(bfs_queue_t *q) {
  q->head = q->tail = 0;
}

static inline int bfs_q_empty(bfs_queue_t *q) {
  return q->head == q->tail;
}

static inline int bfs_q_full(bfs_queue_t *q) {
  return ((q->tail + 1) % BFS_QUEUE_SIZE) == q->head;
}

static inline int bfs_q_enqueue(bfs_queue_t *q, lock_class_link_t *elem) {
  if (bfs_q_full(q))
    return 0;

  q->items[q->tail] = elem;
  q->tail = (q->tail + 1) % BFS_QUEUE_SIZE;
  return 1;
}

static inline lock_class_link_t *bfs_q_dequeue(bfs_queue_t *q) {
  lock_class_link_t *elem;

  if (bfs_q_empty(q))
    return NULL;

  elem = q->items[q->head];
  q->head = (q->head + 1) % BFS_QUEUE_SIZE;
  return elem;
}

static inline void lockdep_lock(void) {
  spin_lock(&main_lock);
}

static inline void lockdep_unlock(void) {
  spin_unlock(&main_lock);
}

static void bfs_init(void) {
  for (int i = 0; i < class_cnt; i++) {
    lock_classes[i].bfs_visited = 0;
  }
}

static inline lock_class_link_t *bfs_next_link(lock_class_link_t *link) {
  if (link == NULL)
    return NULL;
  return TAILQ_NEXT(link, entry);
}

static inline void bfs_mark_visited(lock_class_t *class) {
  class->bfs_visited = 1;
}

static inline int bfs_is_visited(lock_class_t *class) {
  return class->bfs_visited;
}

/*
 * Check that the dependency graph starting at <src> can lead to
 * <target> or not.
 */
static bool check_path(lock_class_t *src, lock_class_t *target) {
  lock_class_link_t *child, *link = NULL;
  bfs_queue_t *queue = &bfs_queue;

  bfs_q_init(queue);
  bfs_init();

  bfs_q_enqueue(queue, TAILQ_FIRST(&src->locked_after));
  bfs_mark_visited(src);

  while ((link = bfs_next_link(link)) || (link = bfs_q_dequeue(queue))) {
    if (bfs_is_visited(link->to))
      continue;
    bfs_mark_visited(link->to);

    if (link->to == target)
      return 1;

    if ((child = TAILQ_FIRST(&link->to->locked_after))) {
      if (!bfs_q_enqueue(queue, child))
        panic("lockdep: bfs queue overflow");
    }
  }

  return 0;
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

static lock_class_t *get_or_create_class(lock_class_mapping_t *lock) {
  lock_class_t *class;

  /* If the lock doesn't have a key then it is statically allocated. In this
   * case use its address as the key. */
  if (lock->key == NULL)
    lock->key = (void *)lock;

  class = look_up_lock_class(lock);
  if (!class) {
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

  if (check_path(link->to, link->from)) {
    panic("lockdep: cycle between locks %s and %s", link->from->name,
          link->to->name);
  }
}

static int is_lock_held(lock_class_t *class) {
  for (int i = 0; i < thread_self()->td_lock_depth; i++) {
    if (thread_self()->td_held_locks[i].lock_class == class)
      return true;
  }
  return false;
}

void lockdep_init(void) {
  for (int i = 0; i < CLASSHASH_SIZE; i++) {
    TAILQ_INIT(&lock_hashtbl[i]);
  }
}

void lockdep_acquire(lock_class_mapping_t *lock) {
  lock_class_t *class;
  held_lock_t *hlock;

  lockdep_lock();

  if (thread_self()->td_lock_depth >= MAX_LOCK_DEPTH)
    panic("lockdep: max lock depth reached");

  if (!(class = lock->lock_class))
    class = get_or_create_class(lock);

  if (!is_lock_held(class)) {
    hlock = &thread_self()->td_held_locks[thread_self()->td_lock_depth++];
    hlock->lock_class = class;
    add_prev_link();
  }

  lockdep_unlock();
}

void lockdep_release(void) {
  lockdep_lock();

  thread_self()->td_lock_depth--;

  if (thread_self()->td_lock_depth < 0)
    panic("lockdep: depth below 0");

  lockdep_unlock();
}

#endif
