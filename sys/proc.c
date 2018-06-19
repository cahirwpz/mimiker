#include <proc.h>
#include <malloc.h>
#include <thread.h>
#include <sysinit.h>
#include <klog.h>
#include <errno.h>
#include <filedesc.h>
#include <wait.h>
#include <signal.h>
#include <sched.h>

static MALLOC_DEFINE(M_PROC, "proc", 1, 2);

static mtx_t all_proc_list_mtx = MTX_INITIALIZER(MTX_DEF);
static proc_list_t all_proc_list = TAILQ_HEAD_INITIALIZER(all_proc_list);
static mtx_t zombie_proc_list_mtx = MTX_INITIALIZER(MTX_DEF);
static proc_list_t zombie_proc_list = TAILQ_HEAD_INITIALIZER(zombie_proc_list);

static mtx_t last_pid_mtx = MTX_INITIALIZER(MTX_DEF);
static pid_t last_pid = 0;

proc_t *proc_create(void) {
  proc_t *proc = kmalloc(M_PROC, sizeof(proc_t), M_ZERO);
  mtx_init(&proc->p_lock, MTX_DEF);
  proc->p_state = PRS_NORMAL;
  TAILQ_INIT(&proc->p_children);

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
  p->p_thread = td;
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

int proc_reap(proc_t *child, int *status) {
  /* Child is now a zombie. Gather its data, cleanup & free. */
  mtx_lock(&child->p_lock);

  assert(child->p_state == PRS_ZOMBIE);

  /* We should have the only reference to the zombie child now, we're about to
     free it. I don't think it may ever happen that there would be multiple
     references to a terminated process, but if it does, we would need to
     introduce refcounting for processes. */
  if (status)
    *status = child->p_exitstatus;

  int retval = child->p_pid;

  if (child->p_parent) {
    WITH_MTX_LOCK (&child->p_parent->p_lock)
      TAILQ_REMOVE(&child->p_parent->p_children, child, p_child);
  }

  WITH_MTX_LOCK (&zombie_proc_list_mtx)
    TAILQ_REMOVE(&zombie_proc_list, child, p_zombie);

  kfree(M_PROC, child);

  return retval;
}

void proc_exit(int exitstatus) {
  /* NOTE: This is a significantly simplified implementation! We currently
     assume there is only one thread in a process. */

  thread_t *td = thread_self();
  proc_t *p = td->td_proc;
  assert(p);

  bool reap_now = false;

  WITH_MTX_LOCK (&p->p_lock) {
    /* Process orphans. They should become children of PID1, but since we don't
       use an init process yet, I have to make them parent-less (so that can no
       longer refer to this terminated process). */
    proc_t *child;
    TAILQ_FOREACH (child, &p->p_children, p_child) {
      WITH_MTX_LOCK (&child->p_lock) {
        child->p_parent = NULL;
        /* XXX: Assign to init, and send it a SIGCHLD. */
      }
    }

    /* Clean up process resources. */
    {
      /* Make sure uspace will not get activated by context switch while it's
       * being deleted. */
      vm_map_t *uspace = p->p_uspace;
      p->p_uspace = NULL;
      vm_map_delete(uspace);
    }
    fdtab_release(p->p_fdtable);

    /* Record some process statistics that will stay maintained in zombie
       state. */
    p->p_exitstatus = exitstatus;

    /* Turn the process into a zombie. */
    WITH_MTX_LOCK (&all_proc_list_mtx)
      TAILQ_REMOVE(&all_proc_list, p, p_all);

    p->p_state = PRS_ZOMBIE;

    WITH_MTX_LOCK (&zombie_proc_list_mtx)
      TAILQ_INSERT_TAIL(&zombie_proc_list, p, p_zombie);

    /* Notify the parent, in various ways, about state change. */
    if (p->p_parent) {
      WITH_MTX_LOCK (&p->p_parent->p_lock) {
        sleepq_broadcast(&p->p_parent->p_children);
        sleepq_broadcast(&p->p_state);

        /* If the parent explicitly ignores SIGCHLD, reap child immediately. */
        if (p->p_sigactions[SIGCHLD].sa_handler == SIG_IGN) {
          reap_now = true;
          goto exit;
        }
      }
      /* sig_send must be called with target process lock not acquired. */
      sig_send(p->p_parent, SIGCHLD);
    }
  }

exit:
  /* If process ready for disposal, then reap it immediately. */
  if (reap_now)
    proc_reap(p, NULL);

  /* Can't call [noreturn] thread_exit() from within a WITH scope. */
  /* This thread is the last one in the process to exit. */
  thread_exit();
}

/* These functions aren't very useful, but they clean up code layout by
   splitting multiple levels of nested loops */
static proc_t *child_find_by_pid(proc_t *p, pid_t pid) {
  assert(mtx_owned(&p->p_lock));
  proc_t *child;
  TAILQ_FOREACH (child, &p->p_children, p_child) {
    if (child->p_pid == pid)
      return child;
  }
  return NULL;
}

static proc_t *child_find_by_state(proc_t *p, proc_state_t state) {
  assert(mtx_owned(&p->p_lock));
  proc_t *child;
  TAILQ_FOREACH (child, &p->p_children, p_child) {
    if (child->p_state == state)
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
  assert(p != NULL);

  proc_t *zombie = NULL;

  if (pid == -1) {
    for (;;) {
      /* Search for any zombie children. */
      WITH_MTX_LOCK (&p->p_lock)
        zombie = child_find_by_state(p, PRS_ZOMBIE);

      if (zombie)
        return proc_reap(zombie, status);

      /* No zombie child was found. */
      if (options & WNOHANG)
        return -ECHILD;

      /* Wait until a child changes state. */
      sleepq_wait(&p->p_children, NULL);
    }
  } else {
    proc_t *child = NULL;

    /* Wait for a particular child. */
    WITH_MTX_LOCK (&p->p_lock)
      child = child_find_by_pid(p, pid);

    /* No such process, or the process is not a child. */
    if (child == NULL)
      return -ECHILD;

    for (;;) {
      WITH_MTX_LOCK (&child->p_lock)
        if (child->p_state == PRS_ZOMBIE)
          zombie = child;

      if (zombie)
        return proc_reap(zombie, status);

      if (options & WNOHANG)
        return 0;

      /* Wait until the child changes state. */
      sleepq_wait(&child->p_state, NULL);
    }
  }

  __unreachable();
}
