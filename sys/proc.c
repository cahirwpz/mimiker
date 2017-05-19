#include <proc.h>
#include <malloc.h>
#include <thread.h>
#include <sysinit.h>
#include <klog.h>
#include <errno.h>
#include <filedesc.h>
#include <wait.h>

static MALLOC_DEFINE(M_PROC, "proc", 1, 2);

static mtx_t all_proc_list_mtx;
static proc_list_t all_proc_list;
static mtx_t zombie_proc_list_mtx;
static proc_list_t zombie_proc_list;

static mtx_t last_pid_mtx;
static pid_t last_pid;

static void proc_init() {
  mtx_init(&all_proc_list_mtx, MTX_DEF);
  TAILQ_INIT(&all_proc_list);
  mtx_init(&zombie_proc_list_mtx, MTX_DEF);
  TAILQ_INIT(&zombie_proc_list);
  mtx_init(&last_pid_mtx, MTX_DEF);
  last_pid = 0;
}

proc_t *proc_create() {
  proc_t *proc = kmalloc(M_PROC, sizeof(proc_t), M_ZERO);
  mtx_init(&proc->p_lock, MTX_DEF);
  TAILQ_INIT(&proc->p_threads);
  proc->p_nthreads = 0;
  proc->p_state = PRS_NORMAL;
  TAILQ_INIT(&proc->p_children);

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
  p->p_nthreads += 1;
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

void proc_exit(int exitstatus) {
  /* NOTE: This is a significantly simplified implementation! We currently
     assume there is only one thread in a process. */

  thread_t *td = thread_self();
  proc_t *p = td->td_proc;
  assert(p);

  mtx_lock(&p->p_lock);

  assert(p->p_nthreads == 1);
  /* If the process had other threads, we'd need to wake the sleeping ones,
     request all of them except this one to call thread_exit from
     exc_before_leave (using a TDF_? flag), join all of them to wait until they
     terminate. */

  /* XXX: If this process has any unwaited zombie children, assign them for
     adoption by pid1 (init), who will wait for them. */

  /* Clean up process resources. */
  vm_map_delete(p->p_uspace);
  fdtab_release(p->p_fdtable);

  /* Record some process statistics that will stay maintained in zombie
     state. */
  p->p_exitstatus = exitstatus;

  /* Turn the process into a zombie. */
  mtx_lock(&all_proc_list_mtx);
  TAILQ_REMOVE(&all_proc_list, p, p_all);
  mtx_unlock(&all_proc_list_mtx);

  p->p_state = PRS_ZOMBIE;

  mtx_lock(&zombie_proc_list_mtx);
  TAILQ_INSERT_TAIL(&zombie_proc_list, p, p_zombie);
  mtx_unlock(&zombie_proc_list_mtx);

  /* Notify parent possibly waiting for this process to become zombie that it is
     now. */
  if (p->p_parent)
    sleepq_broadcast(&p->p_parent->p_children);
  /* Also notify anyone who waits on our state change. */
  sleepq_broadcast(&p->p_state);

  mtx_unlock(&p->p_lock);

  /* This thread is the last one in the process to exit. */
  thread_exit();
}

/* These functions aren't very useful, but they clean up code layout by
   splitting multiple levels of nested loops */
static proc_t *proc_find_child(proc_t *p, pid_t pid) {
  assert(mtx_owned(&p->p_lock));
  proc_t *child;
  TAILQ_FOREACH (child, &p->p_children, p_child) {
    if (child->p_pid == pid)
      return child;
  }
  return NULL;
}

static proc_t *proc_find_zombiechild(proc_t *p) {
  assert(mtx_owned(&p->p_lock));
  proc_t *child;
  TAILQ_FOREACH (child, &p->p_children, p_child) {
    mtx_scoped_lock(&child->p_lock);
    if (child->p_state == PRS_ZOMBIE)
      return child;
  }
  return NULL;
}

int do_waitpid(pid_t pid, int *status, int options) {
  /* We don't have a concept of process groups yet. */
  if (pid < -1 || pid == 0)
    return -ENOTSUP;

  thread_t *td = thread_self();
  proc_t *p = td->td_proc;
  assert(p);

  proc_t *child = NULL;

  if (pid == -1) {
    mtx_scoped_lock(&p->p_lock);
    while (1) {

      /* Search for any zombie children. */
      child = proc_find_zombiechild(p);
      if (child)
        break;

      /* No zombie child was found. */

      if (options & WNOHANG)
        return -ECHILD;

      /* Wait until a child changes state. */
      mtx_unlock(&p->p_lock);
      sleepq_wait(&p->p_children, "any child state change");
      mtx_lock(&p->p_lock);
    }
  } else {
    /* Wait for a particular child. */
    mtx_lock(&p->p_lock);
    child = proc_find_child(p, pid);
    mtx_unlock(&p->p_lock);

    if (!child) /* No such process, or the process is not a child. */
      return -ECHILD;

    mtx_scoped_lock(&child->p_lock);
    while (child->p_state != PRS_ZOMBIE) {
      if (options & WNOHANG)
        return 0;

      mtx_unlock(&child->p_lock);
      sleepq_wait(&child->p_state, "state change");
      mtx_lock(&child->p_lock);
    }
  }

  /* Child is now a zombie. Gather its data, cleanup & free. */
  mtx_lock(&child->p_lock);
  /* We should have the only reference to the zombie child now, we're about to
     free it. I don't think it may ever happen that there would be multiple
     references to a terminated process, but if it does, we would need to
     introduce refcounting for processes. */
  *status = child->p_exitstatus;
  int retval = child->p_pid;

  mtx_lock(&zombie_proc_list_mtx);
  TAILQ_REMOVE(&zombie_proc_list, child, p_zombie);
  mtx_unlock(&zombie_proc_list_mtx);

  kfree(M_PROC, child);

  return retval;
}

SYSINIT_ADD(proc, proc_init, NODEPS);
