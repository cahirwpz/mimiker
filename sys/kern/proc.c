#define KL_LOG KL_PROC
#include <sys/libkern.h>
#include <sys/syslimits.h>
#include <sys/klog.h>
#include <sys/proc.h>
#include <sys/pool.h>
#include <sys/thread.h>
#include <sys/klog.h>
#include <sys/errno.h>
#include <sys/filedesc.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/sleepq.h>
#include <sys/sched.h>
#include <sys/malloc.h>
#include <sys/vfs.h>
#include <bitstring.h>

#define NPROC 64 /* maximum number of processes */
#define CHILDREN(p) (&(p)->p_children)

static POOL_DEFINE(P_PROC, "proc", sizeof(proc_t));
static POOL_DEFINE(P_PGRP, "pgrp", sizeof(pgrp_t));

static mtx_t *all_proc_mtx = &MTX_INITIALIZER(0);

/* all_proc_mtx protects following data: */
static proc_list_t proc_list = TAILQ_HEAD_INITIALIZER(proc_list);
static proc_list_t zombie_list = TAILQ_HEAD_INITIALIZER(zombie_list);
static pgrp_list_t pgrp_list = TAILQ_HEAD_INITIALIZER(pgrp_list);

/* Pid 0 is never available, because of its special treatment by some
 * syscalls e.g. kill. */
static bitstr_t pid_used[bitstr_size(NPROC)] = {1};

/* Process ID management functions */
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

/* Finds process group with the ID specified by pgid or returns NULL. */
static pgrp_t *pgrp_lookup(pgid_t pgid) {
  assert(mtx_owned(all_proc_mtx));

  pgrp_t *pgrp;
  TAILQ_FOREACH (pgrp, &pgrp_list, pg_link)
    if (pgrp->pg_id == pgid)
      return pgrp;
  return NULL;
}

/* Make process leaves its process group. */
static void pgrp_leave(proc_t *p) {
  /* We don't want for any process to see that our p_pgrp is NULL. */
  assert(mtx_owned(all_proc_mtx));

  pgrp_t *pgrp = p->p_pgrp;

  WITH_MTX_LOCK (&pgrp->pg_lock) {
    TAILQ_REMOVE(&p->p_pgrp->pg_members, p, p_pglist);
    p->p_pgrp = NULL;
  }

  if (TAILQ_EMPTY(&pgrp->pg_members)) {
    TAILQ_REMOVE(&pgrp_list, pgrp, pg_link);
    pool_free(P_PGRP, pgrp);
  }
}

int pgrp_enter(proc_t *p, pgid_t pgid) {
  SCOPED_MTX_LOCK(all_proc_mtx);

  pgrp_t *target = pgrp_lookup(pgid);

  /* Only init process will skip that step. */
  if (p->p_pgrp) {
    /* We're done if already belong to the group. */
    if (target == p->p_pgrp)
      return 0;
    /* Leave current group. */
    pgrp_leave(p);
  }

  /* Create new group if one does not exist. */
  if (!target) {
    target = pool_alloc(P_PGRP, M_ZERO);

    TAILQ_INIT(&target->pg_members);
    target->pg_lock = MTX_INITIALIZER(0);
    target->pg_id = pgid;

    TAILQ_INSERT_HEAD(&pgrp_list, target, pg_link);
  }

  /* Subscribe to new or already existing group. */
  WITH_MTX_LOCK (&target->pg_lock) {
    TAILQ_INSERT_HEAD(&target->pg_members, p, p_pglist);
    p->p_pgrp = target;
  }

  return 0;
}

/* Process functions */
proc_t *proc_self(void) {
  return thread_self()->td_proc;
}

void proc_lock(proc_t *p) {
  mtx_lock(&p->p_lock);
}

void proc_unlock(proc_t *p) {
  mtx_unlock(&p->p_lock);
}

proc_t *proc_create(thread_t *td, proc_t *parent) {
  proc_t *p = pool_alloc(P_PROC, M_ZERO);

  mtx_init(&p->p_lock, 0);
  p->p_state = PS_NORMAL;
  p->p_thread = td;
  p->p_parent = parent;

  if (parent && parent->p_elfpath)
    p->p_elfpath = kstrndup(M_STR, parent->p_elfpath, PATH_MAX);

  TAILQ_INIT(CHILDREN(p));

  WITH_MTX_LOCK (&td->td_lock)
    td->td_proc = p;

  return p;
}

void proc_add(proc_t *p) {
  WITH_MTX_LOCK (all_proc_mtx) {
    p->p_pid = pid_alloc();
    TAILQ_INSERT_TAIL(&proc_list, p, p_all);
    if (p->p_parent)
      TAILQ_INSERT_TAIL(CHILDREN(p->p_parent), p, p_child);
  }

  klog("Process PID(%d) {%p} has been created", p->p_pid, p);
}

proc_t *proc_find(pid_t pid) {
  assert(mtx_owned(all_proc_mtx));

  proc_t *p = NULL;
  TAILQ_FOREACH (p, &proc_list, p_all) {
    proc_lock(p);
    if (p->p_pid == pid) {
      /* Skip process if it is not alive. */
      if (!proc_is_alive(p)) {
        proc_unlock(p);
        return NULL;
      }
      break;
    }
    proc_unlock(p);
  }
  return p;
}

int proc_getpgid(pid_t pid, pgid_t *pgidp) {
  SCOPED_MTX_LOCK(all_proc_mtx);

  proc_t *p = proc_find(pid);
  if (!p)
    return ESRCH;

  *pgidp = p->p_pgrp->pg_id;
  proc_unlock(p);
  return 0;
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
  kfree(M_STR, p->p_elfpath);
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

__noreturn void proc_exit(int exitstatus) {
  proc_t *p = proc_self();

  assert(mtx_owned(&p->p_lock));

  /* Mark this process as dying, so others don't attempt to disturb it. */
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

  proc_unlock(p);

  WITH_MTX_LOCK (all_proc_mtx) {
    /* Process orphans, but firstly find init process. */
    proc_t *init;
    TAILQ_FOREACH (init, &proc_list, p_all) {
      if (init->p_pid == 1)
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

    bool auto_reap;
    WITH_MTX_LOCK (&parent->p_lock) {
      auto_reap = parent->p_sigactions[SIGCHLD].sa_handler == SIG_IGN;
      if (!auto_reap)
        sig_kill(parent, SIGCHLD);
    }

    /* We unconditionally notify the parent if they're waiting for a child,
     * even when we reap ourselves, because we might be the last child
     * of the parent, in which case the parent's waitpid should fail,
     * which it can't do if the parent is still waiting.
     * NOTE: If auto_reap is true, we must NOT drop all_proc_mtx
     * between this point and the auto-reap! */
    cv_broadcast(&parent->p_waitcv);

    klog("Turning PID(%d) into zombie!", p->p_pid);

    /* Turn the process into a zombie. */
    WITH_PROC_LOCK(p) {
      p->p_state = PS_ZOMBIE;
    }

    klog("Process PID(%d) {%p} is dead!", p->p_pid, p);

    if (auto_reap) {
      klog("Auto-reaping process PID(%d)!", p->p_pid);
      proc_reap(p);
    }
  }

  /* Can't call [noreturn] thread_exit() from within a WITH scope. */
  /* This thread is the last one in the process to exit. */
  thread_exit();
}

int proc_sendsig(pid_t pid, signo_t sig) {
  SCOPED_MTX_LOCK(all_proc_mtx);

  proc_t *target;

  if (pid > 0) {
    target = proc_find(pid);
    if (target == NULL)
      return EINVAL;
    sig_kill(target, sig);
    proc_unlock(target);
    return 0;
  }

  /* TODO send sig to every process for which the calling process has permission
   * to send signals, except init process */
  if (pid == -1)
    return ENOTSUP;

  pgrp_t *pgrp = NULL;

  if (pid == 0)
    pgrp = proc_self()->p_pgrp;

  if (pid < -1) {
    pgrp = pgrp_lookup(-pid);
    if (!pgrp)
      return EINVAL;
  }

  WITH_MTX_LOCK (&pgrp->pg_lock) {
    TAILQ_FOREACH (target, &pgrp->pg_members, p_pglist) {
      WITH_MTX_LOCK (&target->p_lock)
        sig_kill(target, sig);
    }
  }

  return 0;
}

static bool is_zombie(proc_t *p) {
  return p->p_state == PS_ZOMBIE;
}

/* Wait for direct children. */
int do_waitpid(pid_t pid, int *status, int options, pid_t *cldpidp) {
  proc_t *p = proc_self();

  WITH_MTX_LOCK (all_proc_mtx) {
    for (;;) {
      proc_t *child = NULL;
      /* Whether we found any children meeting criteria implied by pid. */
      bool any = false;

      /* Check children meeting criteria implied by pid. */
      TAILQ_FOREACH (child, CHILDREN(p), p_child) {
        /* pid > 0 => child with PID same as pid */
        if (!(pid == child->p_pid ||
              /* pid == -1 => any child  */
              pid == -1 ||
              /* pid == 0 => child with PGID same as ours */
              ((pid == 0) && (child->p_pgrp == p->p_pgrp)) ||
              /* pid < -1 => child with PGID equal to -pid */
              (pid < -1 && (child->p_pgrp->pg_id != -pid))))
          continue;

        any = true;

        if (is_zombie(child)) {
          *status = child->p_exitstatus;
          *cldpidp = child->p_pid;
          proc_reap(child);
          return 0;
        }

        if (pid > 0) {
          break;
        }
      }

      /* No child meeting criteria specified by pid. */
      if (!any)
        return ECHILD;

      if (options & WNOHANG) {
        *cldpidp = 0;
        return 0;
      }

      /* Wait until one of children changes a state. */
      klog("PID(%d) waits for children (pid = %d)", p->p_pid, pid);
      cv_wait(&p->p_waitcv, all_proc_mtx);
    }
  }

  __unreachable();
}
