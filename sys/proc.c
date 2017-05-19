#include <proc.h>
#include <malloc.h>
#include <thread.h>
#include <sysinit.h>

static MALLOC_DEFINE(M_PROC, "proc", 1, 2);

static mtx_t all_proc_list_mtx = MUTEX_INITIALIZER(MTX_DEF);
static proc_list_t all_proc_list = TAILQ_HEAD_INITIALIZER(all_proc_list);

static mtx_t last_pid_mtx = MUTEX_INITIALIZER(MTX_DEF);
static pid_t last_pid = 0;

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
  SCOPED_MTX_LOCK(&p->p_lock);
  SCOPED_MTX_LOCK(&td->td_lock);

  td->td_proc = p;
  TAILQ_INSERT_TAIL(&p->p_threads, td, td_procq);
  p->p_nthreads += 1;
}

proc_t *proc_find(pid_t pid) {
  SCOPED_MTX_LOCK(&all_proc_list_mtx);

  proc_t *p = NULL;
  TAILQ_FOREACH (p, &all_proc_list, p_all) {
    if (p->p_pid == pid)
      break;
  }
  return p;
}
