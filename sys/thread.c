#define KL_LOG KL_THREAD
#include <klog.h>
#include <stdc.h>
#include <malloc.h>
#include <thread.h>
#include <context.h>
#include <interrupt.h>
#include <pcpu.h>
#include <sync.h>
#include <sched.h>
#include <filedesc.h>

static MALLOC_DEFINE(M_THREAD, "thread", 1, 2);

typedef TAILQ_HEAD(, thread) thread_list_t;

static mtx_t all_threads_mtx = MUTEX_INITIALIZER(MTX_DEF);
static thread_list_t all_threads = TAILQ_HEAD_INITIALIZER(all_threads);

static mtx_t zombie_threads_mtx = MUTEX_INITIALIZER(MTX_DEF);
static thread_list_t zombie_threads = TAILQ_HEAD_INITIALIZER(zombie_threads);

/* FTTB such a primitive method of creating new TIDs will do. */
static tid_t make_tid(void) {
  static volatile tid_t tid = 0;
  /* TODO: Synchronization is missing here. */
  return tid++;
}

void thread_reap(void) {
  /* Exit early if there are no zombie threads. This is particularly important
     during kernel startup. The first thread is created before mtx is
     initialized! Luckily, we don't need to lock it to check whether the list is
     empty. */
  if (TAILQ_EMPTY(&zombie_threads))
    return;

  thread_list_t thq;

  WITH_MTX_LOCK (&zombie_threads_mtx) {
    thq = zombie_threads;
    TAILQ_INIT(&zombie_threads);
  }

  thread_t *td;
  TAILQ_FOREACH (td, &thq, td_zombieq) {
    klog("Reaping thread %ld (%s)", td->td_tid, td->td_name);
    thread_delete(td);
  }
}

thread_t *thread_create(const char *name, void (*fn)(void *), void *arg) {

  thread_reap();

  thread_t *td = kmalloc(M_THREAD, sizeof(thread_t), M_ZERO);

  td->td_sleepqueue = sleepq_alloc();
  td->td_name = kstrndup(M_THREAD, name, TD_NAME_MAX);
  td->td_tid = make_tid();
  td->td_kstack_obj = pm_alloc(1);
  td->td_kstack.stk_base = (void *)PG_VADDR_START(td->td_kstack_obj);
  td->td_kstack.stk_size = PAGESIZE;

  mtx_init(&td->td_lock, MTX_RECURSE);
  cv_init(&td->td_waitcv, "td_waitcv");

  ctx_init(td, fn, arg);

  /* Do not lock the mutex if this call to thread_create was done before any
     threads exists (from thread_bootstrap). Locking the mutex would cause
     problems because during thread bootstrap the current thread is NULL, so a
     fresh unused mutex looks like it is already owned (because mtx->owner also
     is NULL). The proper solution to this problem would be either to use a
     dummy value for PCPU(currthread) during thread_bootstrap, or to use a
     non-NULL (e.g. -1) value for unowned mutexes. Both options require
     discussion, so let's handle this case manually for now. */
  if (thread_self() != NULL)
    mtx_lock(&all_threads_mtx);
  TAILQ_INSERT_TAIL(&all_threads, td, td_all);
  if (thread_self() != NULL)
    mtx_unlock(&all_threads_mtx);

  td->td_state = TDS_READY;
  klog("Thread '%s' {%p} has been created.", td->td_name, td);

  return td;
}

void thread_delete(thread_t *td) {
  assert(td != NULL);
  assert(td != thread_self());
  assert(td->td_sleepqueue != NULL);

  mtx_lock(&all_threads_mtx);
  TAILQ_REMOVE(&all_threads, td, td_all);
  mtx_unlock(&all_threads_mtx);

  pm_free(td->td_kstack_obj);

  sleepq_destroy(td->td_sleepqueue);
  kfree(M_THREAD, td->td_name);
  kfree(M_THREAD, td);
}

thread_t *thread_self(void) {
  return PCPU_GET(curthread);
}

/* For now this is only a stub */
noreturn void thread_exit(void) {
  thread_t *td = thread_self();

  klog("Thread '%s' {%p} has finished.", td->td_name, td);

  mtx_lock(&td->td_lock);

  /* Thread must not exit while in critical section! However, we can't use
     assert here, because assert also calls thread_exit. Thus, in case this
     condition is not met, we'll log the problem, and try to fix the problem. */
  if (td->td_csnest != 0) {
    klog("ERROR: Thread must not exit within a critical section!");
    while (td->td_csnest--)
      critical_leave();
  }

  WITH_MTX_LOCK (&zombie_threads_mtx)
    TAILQ_INSERT_TAIL(&zombie_threads, td, td_zombieq);

  CRITICAL_SECTION {
    td->td_state = TDS_INACTIVE;
    cv_broadcast(&td->td_waitcv);
    mtx_unlock(&td->td_lock);
  }

  sched_switch();

  /* sched_yield will return immediately when scheduler is not active */
  while (true)
    ;
}

void thread_join(thread_t *p) {
  thread_t *td = thread_self();
  thread_t *otd = p;
  klog("Joining '%s' {%p} with '%s' {%p}", td->td_name, td, otd->td_name, otd);

  SCOPED_MTX_LOCK(&otd->td_lock);

  while (otd->td_state != TDS_INACTIVE)
    cv_wait(&otd->td_waitcv, &otd->td_lock);
}

/* It would be better to have a hash-map from tid_t to thread_t,
 * but using a list is sufficient for now. */
thread_t *thread_get_by_tid(tid_t id) {
  SCOPED_MTX_LOCK(&all_threads_mtx);

  thread_t *td = NULL;
  TAILQ_FOREACH (td, &all_threads, td_all) {
    if (td->td_tid == id)
      break;
  }
  return td;
}
