#include <proc.h>
#include <malloc.h>
#include <thread.h>

static MALLOC_DEFINE(M_PROC, "proc", 1, 2);

static mtx_t all_proc_list_mtx;
static proc_list_t all_proc_list;

static mtx_t last_pid_mtx;
static pid_t last_pid;

void proc_init() {
  mtx_init(&all_proc_list_mtx, MTX_DEF);
  TAILQ_INIT(&all_proc_list);
  mtx_init(&last_pid_mtx, MTX_DEF);
  last_pid = 0;
}

proc_t *proc_create() {
  proc_t *proc = kmalloc(M_PROC, sizeof(proc_t), M_ZERO);
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

void proc_populate(proc_t *p, thread_t *td) {
  mtx_scoped_lock(&p->p_lock);
  mtx_scoped_lock(&td->td_lock);
  td->td_proc = p;
  TAILQ_INSERT_TAIL(&p->p_threads, td, td_procq);
}

proc_t *proc_find(pid_t pid) {
  proc_t *p = NULL;
  mtx_scoped_lock(&all_proc_list_mtx);
  TAILQ_FOREACH (p, &all_proc_list, p_all) {
    if (p->p_pid == pid)
      break;
  }
  return p;
}
