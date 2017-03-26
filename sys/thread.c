#include <stdc.h>
#include <malloc.h>
#include <thread.h>
#include <context.h>
#include <interrupt.h>
#include <pcpu.h>
#include <sync.h>
#include <sched.h>
#include <filedesc.h>

static MALLOC_DEFINE(td_pool, "kernel threads pool");

typedef TAILQ_HEAD(, thread) thread_list_t;

static mtx_t all_threads_mtx;
static thread_list_t all_threads;

static mtx_t zombie_threads_mtx;
static thread_list_t zombie_threads;

void thread_init() {
  kmalloc_init(td_pool);
  kmalloc_add_pages(td_pool, 2);

  log("Thread init.");
  mtx_init(&all_threads_mtx, MTX_DEF);
  TAILQ_INIT(&all_threads);

  mtx_init(&zombie_threads_mtx, MTX_DEF);
  TAILQ_INIT(&zombie_threads);
}

/* FTTB such a primitive method of creating new TIDs will do. */
static tid_t make_tid() {
  static volatile tid_t tid = 0;
  /* TODO: Synchronization is missing here. */
  return tid++;
}

void thread_reap() {
  /* Exit early if there are no zombie threads. This is particularly important
     during kernel startup. The first thread is created before mtx is
     initialized! Luckily, we don't need to lock it to check whether the list is
     empty. */
  if (TAILQ_EMPTY(&zombie_threads))
    return;

  mtx_lock(&zombie_threads_mtx);
  thread_list_t thq = zombie_threads;
  TAILQ_INIT(&zombie_threads);
  mtx_unlock(&zombie_threads_mtx);

  thread_t *td;
  TAILQ_FOREACH (td, &thq, td_zombieq) {
    log("Reaping thread %ld (%s)", td->td_tid, td->td_name);
    thread_delete(td);
  }
}

thread_t *thread_create(const char *name, void (*fn)(void *), void *arg) {

  thread_reap();

  thread_t *td = kmalloc(td_pool, sizeof(thread_t), M_ZERO);

  td->td_sleepqueue = sleepq_alloc();
  td->td_name = kstrndup(td_pool, name, TD_NAME_MAX);
  td->td_tid = make_tid();
  td->td_kstack_obj = pm_alloc(1);
  td->td_kstack.stk_base = (void *)PG_VADDR_START(td->td_kstack_obj);
  td->td_kstack.stk_size = PAGESIZE;

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

  log("Thread '%s' {%p} has been created.", td->td_name, td);

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
  kfree(td_pool, td->td_name);
  kfree(td_pool, td);
}

thread_t *thread_self() {
  return PCPU_GET(curthread);
}

void thread_switch_to(thread_t *newtd) {
  thread_t *td = thread_self();

  if (newtd == NULL || newtd == td)
    return;

  /* Thread must not switch while in critical section! */
  assert(td->td_csnest == 0);

  log("Switching from '%s' {%p} to '%s' {%p}.", td->td_name, td, newtd->td_name,
      newtd);

  td->td_state = TDS_READY;
  newtd->td_state = TDS_RUNNING;
  ctx_switch(td, newtd);
}

/* For now this is only a stub */
noreturn void thread_exit(int exitcode) {
  thread_t *td = thread_self();

  log("Thread '%s' {%p} has finished.", td->td_name, td);

  /* Thread must not exit while in critical section! However, we can't use
     assert here, because assert also calls thread_exit. Thus, in case this
     condition is not met, we'll log the problem, and try to fix the problem. */
  if (td->td_csnest != 0) {
    log("ERROR: Thread must not exit within a critical section!");
    while (td->td_csnest--)
      critical_leave();
  }

  fdtab_release(td->td_fdtable);

  mtx_lock(&zombie_threads_mtx);

  critical_enter();
  TAILQ_INSERT_TAIL(&zombie_threads, td, td_zombieq);
  td->td_exitcode = exitcode;
  sleepq_broadcast(&td->td_exitcode);
  td->td_state = TDS_INACTIVE;
  critical_leave();

  mtx_unlock(&zombie_threads_mtx);

  sched_yield();

  /* sched_yield will return immediately when scheduler is not active */
  while (true)
    ;
}

void thread_join(thread_t *p) {
  thread_t *td = thread_self();
  thread_t *otd = p;
  log("Joining '%s' {%p} with '%s' {%p}", td->td_name, td, otd->td_name, otd);

  if (otd->td_state == TDS_INACTIVE)
    return;

  sleepq_wait(&otd->td_exitcode, "Joining threads");
}

void thread_dump_all() {
  thread_t *td;
  /* TODO: Using an array as the one below is risky, as it needs to be kept in
     sync with td_state enum. However, this function will be removed very soon,
     because debugger scripts will replace it entirely, so I decided to keep
     this array here. If you see this message in 2017, please either remove this
     function, or move state_names close to td_state enum! */
  const char *state_names[] = {"inactive", "waiting", "ready", "running"};
  kprintf("[thread] All threads:\n");

  mtx_lock(&all_threads_mtx);
  TAILQ_FOREACH (td, &all_threads, td_all) {
    kprintf("[thread]  % 3ld: %p %s, \"%s\"\n", td->td_tid, (void *)td,
            state_names[td->td_state], td->td_name);
  }
  mtx_unlock(&all_threads_mtx);
}

/* It would be better to have a hash-map from tid_t to thread_t,
 * but using a list is sufficient for now. */
thread_t *thread_get_by_tid(tid_t id) {
  thread_t *td = NULL;

  mtx_lock(&all_threads_mtx);
  TAILQ_FOREACH (td, &all_threads, td_all) {
    if (td->td_tid == id)
      break;
  }
  mtx_unlock(&all_threads_mtx);

  return td;
}
