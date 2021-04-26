#include <sys/mutex.h>
#include <sys/queue.h>
#include <sys/thread.h>
#include <sys/filedesc.h>
#include <sys/sched.h>
#include <sys/exception.h>
#include <sys/libkern.h>
#include <sys/vm_map.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/sbrk.h>
#include <sys/cred.h>
#include <sys/mutex.h>
#include <sys/queue.h>

int do_fork(void (*start)(void *), void *arg, pid_t *cldpidp) {
  thread_t *td = thread_self();
  proc_t *parent = td->td_proc;
  char *name = td->td_name;
  int error = 0;

  /* Cannot fork non-user threads. */
  assert(parent);

  if (start == NULL)
    start = (entry_fn_t)user_exc_leave;
  else
    name = "init";

  /* The new thread will get a new kernel stack. There is no need to copy
   * it from the old one as its contents will get discarded anyway.
   * We just prepare the thread's kernel context to a fresh one so that it will
   * continue execution starting from user_exc_leave (which serves as
   * fork_trampoline). */
  thread_t *newtd = thread_create(name, start, arg, td->td_base_prio);

  /* Clone the thread. Since we don't use fork-oriented thread_t layout, we copy
     all necessary fields one-by one for clarity. The new thread is already on
     the all_thread list, has name and tid set. Many fields don't require setup
     as they will be prepared by sched_add. */

  /* Copy user context.. */
  mcontext_copy(newtd->td_uctx, td->td_uctx);
  mcontext_set_retval(newtd->td_uctx, 0, 0);

  /* New thread does not need the exception frame just yet. */
  newtd->td_kframe = NULL;
  newtd->td_onfault = 0;

  newtd->td_wchan = NULL;
  newtd->td_waitpt = NULL;

  newtd->td_prio = td->td_prio;

  newtd->td_sigmask = td->td_sigmask;

  /* Now, prepare a new process. */
  proc_t *child = proc_create(newtd, parent);

  /* Clone credentials.
   * We have to do it with parent::p_lock held.
   * We don't need to hold child::p_lock because child is not visible yet.
   */
  WITH_PROC_LOCK(parent) {
    cred_fork(child, parent);
  }

  /* Clone the entire process memory space. */
  child->p_uspace = vm_map_clone(parent->p_uspace);

  /* Find copied brk segment. */
  WITH_VM_MAP_LOCK (child->p_uspace) {
    child->p_sbrk = vm_map_find_entry(child->p_uspace, SBRK_START);
    child->p_sbrk_end = parent->p_sbrk_end;
  }

  /* Copy the parent descriptor table. */
  /* TODO: Optionally share the descriptor table between processes. */
  child->p_fdtable = fdtab_copy(parent->p_fdtable);

  vnode_hold(parent->p_cwd);
  child->p_cwd = parent->p_cwd;
  child->p_cmask = parent->p_cmask;

  /* Copy signal handler dispatch rules. */
  memcpy(child->p_sigactions, parent->p_sigactions,
         sizeof(child->p_sigactions));

  /* Link the child process into all the structures
   * by which it can be reached from the outside at once. */
  WITH_MTX_LOCK (&all_proc_mtx) {
    /* Enter child into parent's process group.
     * No jobc adjustments are necessary, since the new child has no children
     * of its own, and it's in the same process group as the parent. */
    WITH_MTX_LOCK (&parent->p_pgrp->pg_lock) {
      child->p_pgrp = parent->p_pgrp;
      TAILQ_INSERT_HEAD(&parent->p_pgrp->pg_members, child, p_pglist);
    }
    proc_add(child);
  }

  *cldpidp = child->p_pid;

  /* After this point you cannot access child process without a lock. */
  sched_add(newtd);

  return error;
}
