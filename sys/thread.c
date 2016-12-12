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
  kmalloc_add_arena(td_pool, pm_alloc(1)->vaddr, PAGESIZE);

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

  td->td_name = name;
  td->td_tid = make_tid();
  td->td_kstack_obj = pm_alloc(1);
  td->td_kstack.stk_base = (void *)PG_VADDR_START(td->td_kstack_obj);
  td->td_kstack.stk_size = PAGESIZE;

  ctx_init(td, fn, arg);

  TAILQ_INSERT_TAIL(&all_threads, td, td_all);

  td->td_state = TDS_READY;

  return td;
}

void thread_delete(thread_t *td) {
  assert(td != NULL);
  assert(td != thread_self());

  TAILQ_REMOVE(&all_threads, td, td_all);

  pm_free(td->td_kstack_obj);
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
noreturn void thread_exit() {
  thread_t *td = thread_self();

  log("Thread '%s' {%p} has finished.", td->td_name, td);

  /* Thread must not exit while in critical section! */
  assert(td->td_csnest == 0);

  if (td->td_fdt)
    file_desc_table_destroy(td->td_fdt);

  critical_enter();
  td->td_state = TDS_INACTIVE;
  sched_yield();
  critical_leave();

  /* sched_yield will return immediately when scheduler is not active */
  while (true)
    ;
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
  TAILQ_FOREACH (td, &all_threads, td_all) {
    kprintf("[thread]  % 3ld: %p %s, \"%s\"\n", td->td_tid, (void *)td,
            state_names[td->td_state], td->td_name);
  }
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
