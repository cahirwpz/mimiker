#define KL_LOG KL_PROC
#include <klog.h>
#include <proc.h>
#include <pool.h>
#include <thread.h>
#include <klog.h>
#include <errno.h>
#include <filedesc.h>
#include <wait.h>
#include <signal.h>
#include <sleepq.h>
#include <sched.h>
#include <malloc.h>

static POOL_DEFINE(P_PROC, "proc", sizeof(proc_t));
static POOL_DEFINE(P_PGRP, "pgrp", sizeof(pgrp_t));

static mtx_t *all_proc_mtx = &MTX_INITIALIZER(0);

/* proc_list, zombie_list and last_pid must be protected by all_proc_mtx */
static proc_list_t proc_list = TAILQ_HEAD_INITIALIZER(proc_list);
static proc_list_t zombie_list = TAILQ_HEAD_INITIALIZER(zombie_list);

static pgrp_list_t pgrp_list = LIST_HEAD_INITIALIZER(pgrp_list);

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

/* Process group functions */

pgrp_t *pgrp_lookup(pgid_t pgid) {
  assert(mtx_owned(all_proc_mtx));

  pgrp_t *pgrp;
  LIST_FOREACH (pgrp, &pgrp_list, pg_link)
    if (pgrp->pg_id == pgid)
      return pgrp;
  return NULL;
}

static void pgrp_leave(proc_t *p) {
  /* We don't want for any process to see that our p_pgrp is NULL. */
  assert(mtx_owned(all_proc_mtx));

  pgrp_t *pgrp = p->p_pgrp;

  WITH_MTX_LOCK (&pgrp->pg_lock) {
    LIST_REMOVE(p, p_pglist);
    p->p_pgrp = NULL;
  }

  if (LIST_EMPTY(&pgrp->pg_members)) {
    LIST_REMOVE(pgrp, pg_link);
    pool_free(P_PGRP, pgrp);
  }
}

int pgrp_enter(proc_t *p, pgid_t pgid) {
  SCOPED_MTX_LOCK(all_proc_mtx);

  pgrp_t *target = pgrp_lookup(pgid);
  if (!target) {
    target = pool_alloc(P_PGRP, PF_ZERO);
    LIST_INIT(&target->pg_members);
    target->pg_lock = MTX_INITIALIZER(0);
    target->pg_id = pgid;

    LIST_INSERT_HEAD(&pgrp_list, target, pg_link);
  }

  pgrp_t *pgrp = p->p_pgrp;

  if (pgrp == target)
    return 0;

  if (pgrp)
    pgrp_leave(p);

  WITH_MTX_LOCK (&target->pg_lock) {
    LIST_INSERT_HEAD(&target->pg_members, p, p_pglist);
    p->p_pgrp = target;
  }

  return 0;
}

/* Process functions */

void proc_lock(proc_t *p) {
  mtx_lock(&p->p_lock);
}

void proc_unlock(proc_t *p) {
  mtx_unlock(&p->p_lock);
}

proc_t *proc_create(thread_t *td, proc_t *parent) {
  proc_t *p = pool_alloc(P_PROC, PF_ZERO);

  mtx_init(&p->p_lock, 0);
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
  assert(mtx_owned(all_proc_mtx));

  proc_t *p = NULL;
  TAILQ_FOREACH (p, &proc_list, p_all) {
    proc_lock(p);
    if (p->p_pid == pid) {
      /* Skip process if it is not alive. */
      if (p->p_state != PS_NORMAL) {
        proc_unlock(p);
        return NULL;
      }
      break;
    }
    proc_unlock(p);
  }
  return p;
}

/* Release zombie process after parent processed its state. */
static void proc_reap(proc_t *p) {
  assert(mtx_owned(all_proc_mtx));

  assert(p->p_state == PS_ZOMBIE);

  klog("Recycling process PID(%d) {%p}", p->p_pid, p);

  pgrp_leave(p);

  if (p->p_parent)
    TAILQ_REMOVE(CHILDREN(p->p_parent), p, p_child);
  TAILQ_REMOVE(&zombie_list, p, p_zombie);

  pid_free(p->p_pid);
  pool_free(P_PROC, p);
}

static void proc_reparent(proc_t *old_parent, proc_t *new_parent) {
  assert(mtx_owned(all_proc_mtx));

  proc_t *child, *next;
  TAILQ_FOREACH_SAFE (child, CHILDREN(old_parent), p_child, next) {
    child->p_parent = new_parent;
    TAILQ_REMOVE(CHILDREN(old_parent), child, p_child);
    if (new_parent)
      TAILQ_INSERT_TAIL(CHILDREN(new_parent), child, p_child);
  }
}

noreturn void proc_exit(int exitstatus) {
  proc_t *p = proc_self();

  /* Mark this process as dying, so others don't attempt to disturb it. */
  WITH_PROC_LOCK(p) {
    p->p_state = PS_DYING;

    /* Clean up process resources. */
    klog("Freeing process PID(%d) {%p} resources", p->p_pid, p);

    /* Detach main thread from the process. */
    p->p_thread = NULL;

    /* Make sure address space won't get activated by context switch while it's
     * being deleted. */
    vm_map_t *uspace = p->p_uspace;
    p->p_uspace = NULL;
    vm_map_delete(uspace);

    fdtab_drop(p->p_fdtable);

    /* Record process statistics that will stay maintained in zombie state. */
    p->p_exitstatus = exitstatus;
  }

  WITH_MTX_LOCK (all_proc_mtx) {
    /* Process orphans, but firstly find init process. */
    proc_t *init;
    TAILQ_FOREACH (init, &proc_list, p_all) {
      if (init->p_pid == 0)
        break;
    }
    proc_reparent(p, init);

    TAILQ_REMOVE(&proc_list, p, p_all);
    TAILQ_INSERT_TAIL(&zombie_list, p, p_zombie);

    /* When the process is dead we can finally signal the parent. */
    proc_t *parent = p->p_parent;
    if (!parent)
      panic("'init' process died!");

    klog("Wakeup PID(%d) because child PID(%d) died", parent->p_pid, p->p_pid);

    cv_broadcast(&parent->p_waitcv);
    proc_lock(parent);
    sig_kill(parent, SIGCHLD);

    klog("Turning PID(%d) into zombie!", p->p_pid);

    /* Turn the process into a zombie. */
    WITH_PROC_LOCK(p) {
      p->p_state = PS_ZOMBIE;
    }

    klog("Process PID(%d) {%p} is dead!", p->p_pid, p);
  }

  /* Can't call [noreturn] thread_exit() from within a WITH scope. */
  /* This thread is the last one in the process to exit. */
  thread_exit();
}

int proc_sendsig(pid_t pid, signo_t sig) {
  SCOPED_MTX_LOCK(all_proc_mtx);

  proc_t *target = proc_find(pid);
  if (target == NULL)
    return -EINVAL;
  sig_kill(target, sig);
  return 0;
}

/* Wait for direct children. */
int do_waitpid(pid_t pid, int *status, int options) {
  proc_t *p = proc_self();

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
        /* Search within zombie childrens. */
        TAILQ_FOREACH (zombie, CHILDREN(p), p_child) {
          if (pid < -1 && zombie->p_pgrp->pg_id != -pid)
            continue;
          if (pid == 0 && zombie->p_pgrp->pg_id != p->p_pgrp->pg_id)
            continue;
          if (zombie->p_state == PS_ZOMBIE)
            break;
        }
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
