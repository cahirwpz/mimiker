#include <proc.h>
#include <malloc.h>
#include <thread.h>

static MALLOC_DEFINE(proc_pool, "proc pool");

static mtx_t all_proc_list_mtx;
static TAILQ_HEAD(, proc) all_proc_list;

static mtx_t last_pid_mtx;
static pid_t last_pid;

void proc_init() {
  kmalloc_init(proc_pool, 2, 2);
  mtx_init(&all_proc_list_mtx, MTX_DEF);
  TAILQ_INIT(&all_proc_list);
  mtx_init(&last_pid_mtx, MTX_DEF);
  last_pid = 0;
}

proc_t *proc_create() {
  proc_t *proc = kmalloc(proc_pool, sizeof(proc_t), M_ZERO);
  mtx_init(&proc->p_lock, MTX_DEF);
  TAILQ_INIT(&proc->p_threads);

  mtx_lock(&all_proc_list_mtx);
  TAILQ_INSERT_TAIL(&all_proc_list, proc, p_all);
  mtx_unlock(&all_proc_list_mtx);

  mtx_lock(&last_pid_mtx);
  proc->p_pid = last_pid++;
  mtx_unlock(&last_pid_mtx);

  return proc;
}

void proc_add_thread(proc_t *p, thread_t *td) {
  mtx_scoped_lock(&p->p_lock);
  mtx_scoped_lock(&td->td_lock);
  td->td_proc = p;
  TAILQ_INSERT_TAIL(&p->p_threads, td, td_procthreadq);
}
