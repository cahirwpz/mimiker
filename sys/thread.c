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
/* TODO: Synchronize access to the list */
static thread_list_t all_threads;

void thread_init() {
  kmalloc_init(td_pool);
  kmalloc_add_pages(td_pool, 2);

  TAILQ_INIT(&all_threads);
}

/* FTTB such a primitive method of creating new TIDs will do. */
static tid_t make_tid() {
  static volatile tid_t tid = 0;
  /* TODO: Synchronization is missing here. */
  return tid++;
}

thread_t *thread_create(const char *name, void (*fn)(void *), void *arg) {
  thread_t *td = kmalloc(td_pool, sizeof(thread_t), M_ZERO);

  td->td_sleepqueue = sleepq_alloc();
  td->td_name = kstrndup(td_pool, name, TD_NAME_MAX);
  td->td_tid = make_tid();
  td->td_kstack_obj = pm_alloc(1);
  td->td_kstack.stk_base = (void *)PG_VADDR_START(td->td_kstack_obj);
  td->td_kstack.stk_size = PAGESIZE;

  ctx_init(td, fn, arg);

  TAILQ_INSERT_TAIL(&all_threads, td, td_all);

  td->td_state = TDS_READY;

  log("Thread '%s' {%p} has been created.", td->td_name, td);

  return td;
}

void thread_delete(thread_t *td) {
  assert(td != NULL);
  assert(td != thread_self());
  assert(td->td_sleepqueue != NULL);

  TAILQ_REMOVE(&all_threads, td, td_all);

  pm_free(td->td_kstack_obj);

  sleepq_destroy(td->td_sleepqueue);
  kfree(td_pool, td->td_name);
  kfree(td_pool, td);
}

thread_t *thread_self() {
  return PCPU_GET(curthread);
}

/* For now this is only a stub */
noreturn void thread_exit() {
  thread_t *td = thread_self();

  log("Thread '%s' {%p} has finished.", td->td_name, td);

  /* Thread must not exit while in critical section! */
  assert(td->td_csnest == 0);

  fdtab_release(td->td_fdtable);

  critical_enter();
  td->td_state = TDS_INACTIVE;
  sched_yield();
  critical_leave();

  /* sched_yield will return immediately when scheduler is not active */
  while (true)
    ;
}

/* It would be better to have a hash-map from tid_t to thread_t,
 * but using a list is sufficient for now. */
thread_t *thread_get_by_tid(tid_t id) {
  thread_t *td = NULL;
  TAILQ_FOREACH (td, &all_threads, td_all) {
    if (td->td_tid == id)
      break;
  }
  return td;
}
