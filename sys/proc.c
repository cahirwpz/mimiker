#include <proc.h>
#include <malloc.h>
#include <thread.h>
#include <sysinit.h>
#include <klog.h>
#include <errno.h>
#include <filedesc.h>
#include <wait.h>
#include <signal.h>

static MALLOC_DEFINE(M_PROC, "proc", 1, 2);

static mtx_t all_proc_list_mtx = MUTEX_INITIALIZER(MTX_DEF);
static proc_list_t all_proc_list = TAILQ_HEAD_INITIALIZER(all_proc_list);
static mtx_t zombie_proc_list_mtx = MUTEX_INITIALIZER(MTX_DEF);
static proc_list_t zombie_proc_list = TAILQ_HEAD_INITIALIZER(zombie_proc_list);

static mtx_t last_pid_mtx = MUTEX_INITIALIZER(MTX_DEF);
static pid_t last_pid = 0;

proc_t *proc_create(void) {
  proc_t *proc = kmalloc(M_PROC, sizeof(proc_t), M_ZERO);
  mtx_init(&proc->p_lock, MTX_DEF);
  TAILQ_INIT(&proc->p_threads);
  proc->p_nthreads = 0;
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

/* Always call proc_reap with parent process locked. */
static int proc_reap(proc_t *child, int *status) {
  /* Child is now a zombie. Gather its data, cleanup & free. */
  mtx_lock(&child->p_lock);

  /* We should have the only reference to the zombie child now, we're about to
     free it. I don't think it may ever happen that there would be multiple
     references to a terminated process, but if it does, we would need to
     introduce refcounting for processes. */
  *status = child->p_exitstatus;

  int retval = child->p_pid;

  assert(&child->p_parent);
  assert(mtx_owned(&child->p_parent->p_lock));
  TAILQ_REMOVE(&child->p_parent->p_children, child, p_child);

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

  WITH_MTX_LOCK (&p->p_lock) {

    assert(p->p_nthreads == 1);
    /* If the process had other threads, we'd need to wake the sleeping ones,
       request all of them except this one to call thread_exit from
       exc_before_leave (using a TDF_? flag), join all of them to wait until
       they terminate. */

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
    vm_map_delete(p->p_uspace);
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
          int ignore_status;
          proc_reap(p, &ignore_status);
          /* Can't call [noreturn] thread_exit() from within a WITH scope. */
          goto exit;
        }
      }
      /* sig_send must be called with target process lock not acquired. */
      sig_send(p->p_parent, SIGCHLD);
    }
  }

exit:
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
    SCOPED_MTX_LOCK(&child->p_lock);
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

  proc_t *child = NULL;

  if (pid == -1) {
    for (;;) {
      /* Search for any zombie children. */
      WITH_MTX_LOCK (&p->p_lock) {
        child = child_find_by_state(p, PRS_ZOMBIE);
        if (child)
          return proc_reap(child, status);
      }

      /* No zombie child was found. */
      if (options & WNOHANG)
        return -ECHILD;

      /* Wait until a child changes state. */
      sleepq_wait(&p->p_children, "any child state change");
    }
  } else {
    /* Wait for a particular child. */
    WITH_MTX_LOCK (&p->p_lock)
      child = child_find_by_pid(p, pid);

    /* No such process, or the process is not a child. */
    if (child == NULL)
      return -ECHILD;

    for (;;) {
      WITH_MTX_LOCK (&child->p_lock)
        if (child->p_state == PRS_ZOMBIE)
          return proc_reap(child, status);

      if (options & WNOHANG)
        return 0;

      /* Wait until the child changes state. */
      sleepq_wait(&child->p_state, "state change");
    }
  }

  __unreachable();
}
