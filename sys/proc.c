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
  proc->p_nthreads = 0;

  WITH_MTX_LOCK (&all_proc_list_mtx)
    TAILQ_INSERT_TAIL(&all_proc_list, proc, p_all);

  WITH_MTX_LOCK (&last_pid_mtx)
    proc->p_pid = last_pid++;

  return proc;
}

void proc_populate(proc_t *p, thread_t *td) {
  WITH_MTX_LOCK (&p->p_lock) {
    WITH_MTX_LOCK (&td->td_lock) {
      td->td_proc = p;
      TAILQ_INSERT_TAIL(&p->p_threads, td, td_procq);
      p->p_nthreads += 1;
    }
  }
}

proc_t *proc_find(pid_t pid) {
  proc_t *p = NULL;
  WITH_MTX_LOCK (&all_proc_list_mtx) {
    TAILQ_FOREACH (p, &all_proc_list, p_all) {
      if (p->p_pid == pid)
        break;
    }
  }
  return p;
}
