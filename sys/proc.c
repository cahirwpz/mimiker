#define KL_LOG KL_PROC
#include <klog.h>
#include <proc.h>
#include <pool.h>
#include <thread.h>
#include <sysinit.h>
#include <klog.h>
#include <errno.h>
#include <filedesc.h>
#include <wait.h>
#include <signal.h>
#include <sleepq.h>
#include <sched.h>

static POOL_DEFINE(P_PROC, "proc", sizeof(proc_t));

static mtx_t *all_proc_mtx = &MTX_INITIALIZER(MTX_DEF);

/* proc_list, zombie_list and last_pid must be protected by all_proc_mtx */
static proc_list_t proc_list = TAILQ_HEAD_INITIALIZER(proc_list);
static proc_list_t zombie_list = TAILQ_HEAD_INITIALIZER(zombie_list);

#define CHILDREN(p) (&(p)->p_children)

proc_t *proc_self(void) {
  return thread_self()->td_proc;
}

#define NPROC 64 /* maximum number of processes */

static bitstr_t pid_used[bitstr_size(NPROC)] = {0};

static pid_t pid_alloc(void) {
  assert(mtx_owned(all_proc_mtx));

  pid_t pid;
  bit_ffc(pid_used, NPROC, &pid);
  if (pid < 0)
    panic("Out of PIDs!");
  bit_set(pid_used, pid);
  return pid;
}

static void pid_free(pid_t pid) {
  assert(mtx_owned(all_proc_mtx));

  bit_clear(pid_used, (unsigned)pid);
}

proc_t *proc_create(thread_t *td, proc_t *parent) {
  proc_t *p = pool_alloc(P_PROC, PF_ZERO);

  mtx_init(&p->p_lock, MTX_DEF);
  p->p_state = PS_NORMAL;
  p->p_thread = td;
  p->p_parent = parent;
  TAILQ_INIT(CHILDREN(p));

  WITH_MTX_LOCK (&td->td_lock)
    td->td_proc = p;

  WITH_MTX_LOCK (all_proc_mtx) {
    p->p_pid = pid_alloc();
    TAILQ_INSERT_TAIL(&proc_list, p, p_all);
    if (parent)
      TAILQ_INSERT_TAIL(CHILDREN(parent), p, p_child);
  }

  klog("Process PID(%d) {%p} has been created", p->p_pid, p);

  return p;
}

proc_t *proc_find(pid_t pid) {
  SCOPED_MTX_LOCK(all_proc_mtx);

  proc_t *p = NULL;
  TAILQ_FOREACH (p, &proc_list, p_all) {
    mtx_lock(&p->p_lock);
    if (p->p_pid == pid)
      break;
    mtx_unlock(&p->p_lock);
  }
  return p;
}

/* Release zombie process after parent processed its state. */
static void proc_reap(proc_t *p) {
  assert(mtx_owned(all_proc_mtx));

  assert(p->p_state == PS_ZOMBIE);

  klog("Recycling process PID(%d) {%p}", p->p_pid, p);

  if (p->p_parent)
    TAILQ_REMOVE(CHILDREN(p->p_parent), p, p_child);
  TAILQ_REMOVE(&zombie_list, p, p_zombie);

  pid_free(p->p_pid);
  pool_free(P_PROC, p);
}

static void proc_reparent(proc_t *child, proc_t *parent) {
  child->p_parent = NULL;
}

noreturn void proc_exit(int exitstatus) {
  proc_t *p = proc_self();

  /* Mark this process as dying, so others don't attempt to disturb it. */
  WITH_MTX_LOCK (&p->p_lock)
    p->p_state = PS_DYING;

  WITH_MTX_LOCK (&p->p_lock) {
    /* Clean up process resources. */
    klog("Freeing process PID(%d) {%p} resources", p->p_pid, p);

    /* Detach main thread from the process. */
    p->p_thread = NULL;

    /* Make sure address space won't get activated by context switch while it's
     * being deleted. */
    vm_map_t *uspace = p->p_uspace;
    p->p_uspace = NULL;
    vm_map_delete(uspace);

    fdtab_release(p->p_fdtable);

    /* Record process statistics that will stay maintained in zombie state. */
    p->p_exitstatus = exitstatus;
  }

  WITH_MTX_LOCK (all_proc_mtx) {
    /*
     * Process orphans.
     *
     * XXX: Orphans should become children of init process, but since we don't
     * have one yet let's make them parent-less.
     */
    proc_t *child;
    TAILQ_FOREACH (child, CHILDREN(p), p_child)
      proc_reparent(child, NULL);

    TAILQ_REMOVE(&proc_list, p, p_all);
    TAILQ_INSERT_TAIL(&zombie_list, p, p_zombie);

    /* When the process is dead we can finally signal the parent. */
    proc_t *parent = p->p_parent;

    klog("Wakeup PID(%d) because child died", parent->p_pid);

    cv_broadcast(&parent->p_waitcv);

    if (parent->p_sigactions[SIGCHLD].sa_handler != SIG_IGN) {
      /* sig_send must be called with target process lock not acquired. */
      sig_send(parent, SIGCHLD);
    }

    /* Turn the process into a zombie. */
    WITH_MTX_LOCK (&p->p_lock)
      p->p_state = PS_ZOMBIE;

    klog("Process PID(%d) {%p} is dead!", p->p_pid, p);
  }

  /* Can't call [noreturn] thread_exit() from within a WITH scope. */
  /* This thread is the last one in the process to exit. */
  thread_exit();
}

/* Wait for direct children. */
int do_waitpid(pid_t pid, int *status, int options) {
  proc_t *p = proc_self();

  /* We don't have a concept of process groups yet. */
  if (pid < -1 || pid == 0)
    return -ENOTSUP;

  WITH_MTX_LOCK (all_proc_mtx) {
    proc_t *child = NULL;

    if (pid > 0) {
      /* Wait for a particular child. */
      TAILQ_FOREACH (child, CHILDREN(p), p_child)
        if (child->p_pid == pid)
          break;
      /* No such process, or the process is not a child. */
      if (child == NULL)
        return -ECHILD;
    }

    for (;;) {
      proc_t *zombie = NULL;

      if (child == NULL) {
        /* Search for any zombie children. */
        TAILQ_FOREACH (zombie, CHILDREN(p), p_child)
          if (zombie->p_state == PS_ZOMBIE)
            break;
      } else {
        /* Is the chosen one zombie? */
        if (child->p_state == PS_ZOMBIE)
          zombie = child;
      }

      if (zombie) {
        if (status)
          *status = zombie->p_exitstatus;
        pid_t pid = zombie->p_pid;
        proc_reap(zombie);
        return pid;
      }

      /* No zombie child was found. */
      if (options & WNOHANG)
        return 0;

      /* Wait until one of children changes a state. */
      klog("PID(%d) waits for children", p->p_pid);
      cv_wait(&p->p_waitcv, all_proc_mtx);
    }
  }

  __unreachable();
}
