#define KL_LOG KL_SIGNAL
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/signal.h>
#include <sys/thread.h>
#include <sys/errno.h>
#include <sys/sleepq.h>
#include <sys/proc.h>
#include <sys/wait.h>
#include <sys/sched.h>
#include <sys/spinlock.h>
#include <sys/malloc.h>

static KMALLOC_DEFINE(M_SIGNAL, "signal");

/*!\brief Signal properties.
 *
 * Non-mutually-exclusive properties can be bitwise-ORed together.
 * \note Properties that have set bits in common with #SA_DEFACT_MASK
 * are mutually exclusive and denote the default action of the signal. */
/* clang-format off */
typedef enum {
  SA_IGNORE = 0x1,
  SA_KILL = 0x2,
  SA_CONT = 0x3, /* Continue a stopped process */
  SA_STOP = 0x4, /* Stop a process */
  SA_CANTMASK = 0x8,
  SA_TTYSTOP = 0x10,             /* Stop signal sent by TTY subsystem */
} sigprop_t;

/*!\brief Mask used to extract the default action from a sigprop_t. */
#define SA_DEFACT_MASK 0x7

/* clang-format off */
static const sigprop_t sig_properties[NSIG] = {
  [SIGHUP] = SA_KILL,
  [SIGINT] = SA_KILL,
  [SIGILL] = SA_KILL,
  [SIGABRT] = SA_KILL,
  [SIGFPE] = SA_KILL,
  [SIGSEGV] = SA_KILL,
  [SIGKILL] = SA_KILL | SA_CANTMASK,
  [SIGTERM] = SA_KILL,
  [SIGSTOP] = SA_STOP | SA_CANTMASK,
  [SIGCONT] = SA_CONT,
  [SIGCHLD] = SA_IGNORE,
  [SIGTTIN] = SA_STOP | SA_TTYSTOP,
  [SIGTTOU] = SA_STOP | SA_TTYSTOP,
  [SIGUSR1] = SA_KILL,
  [SIGUSR2] = SA_KILL,
  [SIGBUS] = SA_KILL,
};

static const sigset_t cantmask = {__sigmask(SIGKILL) | __sigmask(SIGSTOP)};

static const char *sig_name[NSIG] = {
  [SIGHUP] = "SIGHUP",
  [SIGINT] = "SIGINT",
  [SIGILL] = "SIGILL",
  [SIGABRT] = "SIGABRT",
  [SIGFPE] = "SIGFPE",
  [SIGSEGV] = "SIGSEGV",
  [SIGKILL] = "SIGKILL",
  [SIGTERM] = "SIGTERM",
  [SIGSTOP] = "SIGSTOP",
  [SIGCONT] = "SIGCONT",
  [SIGCHLD] = "SIGCHLD",
  [SIGTTIN] = "SIGTTIN",
  [SIGTTOU] = "SIGTTOU",
  [SIGUSR1] = "SIGUSR1",
  [SIGUSR2] = "SIGUSR2",
  [SIGBUS] = "SIGBUS",
};
/* clang-format on */

static void sigpend_get(sigpend_t *sp, signo_t sig, ksiginfo_t *out);

/* Default action for a signal. */
static sigprop_t defact(signo_t sig) {
  assert(sig <= NSIG);
  return sig_properties[sig] & SA_DEFACT_MASK;
}

static inline bool sig_ignored(sigaction_t *sigactions, signo_t sig) {
  return (sigactions[sig].sa_handler == SIG_IGN ||
          (sigactions[sig].sa_handler == SIG_DFL &&
           (defact(sig) == SA_IGNORE || sig == SIGCONT)));
}

/* Members of an orphaned process group should ignore
 * TTY stop signals with a default handler. */
static inline bool sig_ignore_ttystop(proc_t *p, signo_t sig) {
  return ((sig_properties[sig] & SA_TTYSTOP) && (p->p_pgrp->pg_jobc == 0) &&
          (p->p_sigactions[sig].sa_handler == SIG_DFL));
}

int do_sigaction(signo_t sig, const sigaction_t *act, sigaction_t *oldact) {
  thread_t *td = thread_self();
  proc_t *p = td->td_proc;

  if (sig >= NSIG)
    return EINVAL;

  if (sig_properties[sig] & SA_CANTMASK)
    return EINVAL;

  WITH_PROC_LOCK(p) {
    if (oldact != NULL)
      memcpy(oldact, &p->p_sigactions[sig], sizeof(sigaction_t));
    if (act != NULL)
      memcpy(&p->p_sigactions[sig], act, sizeof(sigaction_t));
    /* If ignoring a pending signal, discard it. */
    if (sig_ignored(p->p_sigactions, sig))
      sigpend_get(&td->td_sigpend, sig, NULL);
  }

  return 0;
}

static signo_t sig_pending(thread_t *td) {
  proc_t *p = td->td_proc;

  assert(p != NULL);
  assert(mtx_owned(&p->p_lock));

  sigset_t unblocked = td->td_sigpend.sp_set;
  __sigminusset(&td->td_sigmask, &unblocked);

  signo_t ret = __sigfindset(&unblocked);
  assert(ret < NSIG);
  return ret;
}

int do_sigprocmask(int how, const sigset_t *set, sigset_t *oset) {
  proc_t *proc = proc_self();
  thread_t *td = proc->p_thread;
  assert(mtx_owned(&proc->p_lock));

  sigset_t *const mask = &td->td_sigmask;

  if (oset != NULL)
    *oset = *mask;

  if (set == NULL)
    return 0;

  sigset_t nset = *set;
  __sigminusset(&cantmask, &nset);

  if (how == SIG_BLOCK) {
    __sigplusset(&nset, mask);
    return 0;
  }

  if (how == SIG_UNBLOCK) {
    __sigminusset(&nset, mask);
  } else if (how == SIG_SETMASK) {
    *mask = nset;
  } else {
    return EINVAL;
  }

  if (sig_pending(td)) {
    WITH_SPIN_LOCK (td->td_lock)
      td->td_flags |= TDF_NEEDSIGCHK;
  }

  return 0;
}

int do_sigsuspend(proc_t *p, const sigset_t *mask) {
  thread_t *td = thread_self();
  assert(td->td_proc == p);

  assert((td->td_pflags & TDP_OLDSIGMASK) == 0);
  td->td_oldsigmask = td->td_sigmask;
  td->td_pflags |= TDP_OLDSIGMASK;

  WITH_PROC_LOCK(p) {
    do_sigprocmask(SIG_SETMASK, mask, NULL);

    /*
     * We want the sleep to be interrupted only if there's an actual signal
     * to be handled, but _sleepq_wait() returns immediately if TDF_NEEDSIGCHK
     * is set (without checking whether there's an actual pending signal),
     * so we clear the flag here if there are no real pending signals.
     */
    WITH_SPIN_LOCK (td->td_lock) {
      if (sig_pending(td))
        td->td_flags |= TDF_NEEDSIGCHK;
      else
        td->td_flags &= ~TDF_NEEDSIGCHK;
    }
  }

  int error;
  error = sleepq_wait_intr(&td->td_sigmask, "sigsuspend()");
  assert(error == EINTR);

  return EINTR;
}

static ksiginfo_t *ksiginfo_copy(const ksiginfo_t *src) {
  ksiginfo_t *kp = kmalloc(M_SIGNAL, sizeof(ksiginfo_t), M_NOWAIT);
  assert(kp != NULL);

  memcpy(kp, src, sizeof(ksiginfo_t));
  kp->ksi_flags &= ~KSI_QUEUED;
  kp->ksi_flags |= KSI_FROMPOOL;
  return kp;
}

static void ksiginfo_free(ksiginfo_t *ksi) {
  assert((ksi->ksi_flags & KSI_FROMPOOL) != 0);
  assert((ksi->ksi_flags & KSI_QUEUED) == 0);

  kfree(M_SIGNAL, ksi);
}

void sigpend_init(sigpend_t *sp) {
  assert(sp != NULL);
  TAILQ_INIT(&sp->sp_info);
  __sigemptyset(&sp->sp_set);
}

void sigpend_destroy(sigpend_t *sp) {
  assert(sp != NULL);
  __sigemptyset(&sp->sp_set);

  ksiginfo_t *ksi;
  while (!TAILQ_EMPTY(&sp->sp_info)) {
    ksi = TAILQ_FIRST(&sp->sp_info);
    TAILQ_REMOVE(&sp->sp_info, ksi, ksi_list);
    assert(ksi->ksi_flags & KSI_FROMPOOL);
    assert(ksi->ksi_flags & KSI_QUEUED);
    ksi->ksi_flags &= ~KSI_QUEUED;
    ksiginfo_free(ksi);
  }
}

static void sigpend_get(sigpend_t *sp, signo_t sig, ksiginfo_t *out) {
  __sigdelset(&sp->sp_set, sig);

  /* Find siginfo and copy it out. */
  ksiginfo_t *ksi;
  TAILQ_FOREACH (ksi, &sp->sp_info, ksi_list) {
    if (ksi->ksi_signo == sig)
      break;
  }

  if (!ksi) {
    if (out) {
      bzero(out, sizeof(ksiginfo_t));
      out->ksi_signo = sig;
      out->ksi_code = SI_NOINFO;
    }
    return;
  }

  TAILQ_REMOVE(&sp->sp_info, ksi, ksi_list);
  assert(ksi->ksi_flags & KSI_FROMPOOL);
  assert(ksi->ksi_flags & KSI_QUEUED);
  ksi->ksi_flags &= ~KSI_QUEUED;
  if (out) {
    memcpy(out, ksi, sizeof(ksiginfo_t));
    out->ksi_flags &= ~KSI_FROMPOOL;
  }
  ksiginfo_free(ksi);
}

static void sigpend_put(sigpend_t *sp, ksiginfo_t *ksi) {
  assert(sp != NULL);
  assert(ksi != NULL);
  assert(ksi->ksi_flags & KSI_FROMPOOL);
  assert((ksi->ksi_flags & KSI_QUEUED) == 0);

  signo_t sig = ksi->ksi_signo;
  __sigaddset(&sp->sp_set, sig);

  /* If there is no siginfo, we are done. */
  if (ksi->ksi_flags & KSI_RAW) {
    ksiginfo_free(ksi);
    return;
  }

  ksiginfo_t *kp;
  TAILQ_FOREACH (kp, &sp->sp_info, ksi_list) {
    if (kp->ksi_signo == sig)
      break;
  }

  if (kp) {
    kp->ksi_info = ksi->ksi_info;
    kp->ksi_flags = ksi->ksi_flags | KSI_QUEUED;
    ksiginfo_free(ksi);
  } else {
    TAILQ_INSERT_TAIL(&sp->sp_info, ksi, ksi_list);
    ksi->ksi_flags |= KSI_QUEUED;
  }
}

void sig_child(proc_t *p, int code) {
  assert(p != NULL);
  assert(mtx_owned(&p->p_lock));

  proc_t *parent = p->p_parent;
  assert(mtx_owned(&parent->p_lock));

  ksiginfo_t ksi = {
    .ksi_signo = SIGCHLD,
    .ksi_code = code,
    .ksi_pid = p->p_pid,
    .ksi_uid = p->p_cred.cr_euid,
  };

  sig_kill(parent, &ksi);
}

/*
 * NOTE: This is a very simple implementation! Unimplemented features:
 * - Thread tracing and debugging
 * - Multiple threads in a process
 * These limitations (plus the fact that we currently have very little thread
 * states) make the logic of sending a signal very simple!
 */
void sig_kill(proc_t *p, ksiginfo_t *ksi) {
  assert(p != NULL);
  assert(mtx_owned(&p->p_lock));
  assert(ksi != NULL);

  signo_t sig = ksi->ksi_signo;
  assert(sig < NSIG);

  /* Zombie or dying processes shouldn't accept any signals. */
  if (!proc_is_alive(p))
    return;

  thread_t *td = p->p_thread;

  sig_t handler = p->p_sigactions[sig].sa_handler;
  bool continued = sig == SIGCONT || sig == SIGKILL;

  /* If the signal is ignored, don't even bother posting it,
   * unless it's waking up a stopped process. */
  if (p->p_state == PS_STOPPED && continued) {
    p->p_state = PS_NORMAL;
    p->p_flags |= PF_STATE_CHANGED;
    WITH_PROC_LOCK(p->p_parent) {
      proc_wakeup_parent(p->p_parent);
    }
  } else if (handler == SIG_IGN ||
             (defact(sig) == SA_IGNORE && handler == SIG_DFL)) {
    return;
  }

  /* Theoretically, we should hold all_proc_mtx since sig_ignore_ttystop()
   * reads pg_jobc, but the race here is harmless. */
  if (sig_ignore_ttystop(p, sig))
    return;

  /* If stopping or continuing,
   * remove pending signals with the opposite effect. */
  if (defact(sig) == SA_STOP)
    sigpend_get(&td->td_sigpend, SIGCONT, NULL);

  ksiginfo_t *kp = ksiginfo_copy(ksi);

  if (sig == SIGCONT) {
    /* XXX there should be a better way to do this. */
    sigpend_get(&td->td_sigpend, SIGSTOP, NULL);
    sigpend_get(&td->td_sigpend, SIGTTIN, NULL);
    sigpend_get(&td->td_sigpend, SIGTTOU, NULL);

    /* In case of SIGCONT, make it pending only if the process catches it. */
    if (handler != SIG_IGN && handler != SIG_DFL)
      sigpend_put(&td->td_sigpend, kp);
    else
      ksiginfo_free(kp);
  } else {
    /* Every other signal is marked as pending. */
    sigpend_put(&td->td_sigpend, kp);
  }

  /* Don't wake up the target thread if it blocks the signal being sent.
   * Exception: SIGCONT wakes up stopped threads even if it's blocked. */
  if (__sigismember(&td->td_sigmask, sig)) {
    if (continued)
      WITH_SPIN_LOCK (td->td_lock)
        thread_continue(td);
  } else {
    WITH_SPIN_LOCK (td->td_lock) {
      td->td_flags |= TDF_NEEDSIGCHK;
      /* If the thread is sleeping interruptibly (!), wake it up, so that it
       * continues execution and the signal gets delivered soon. */
      if (td_is_interruptible(td)) {
        /* XXX Maybe TDF_NEEDSIGCHK should be protected by a different lock? */
        spin_unlock(td->td_lock);
        sleepq_abort(td); /* Locks & unlocks td_lock */
        spin_lock(td->td_lock);
      } else if (continued) {
        thread_continue(td);
      }
    }
  }
}

void sig_pgkill(pgrp_t *pg, ksiginfo_t *ksi) {
  assert(mtx_owned(&pg->pg_lock));

  proc_t *p;

  TAILQ_FOREACH (p, &pg->pg_members, p_pglist) {
    WITH_PROC_LOCK(p) {
      sig_kill(p, ksi);
    }
  }
}

int sig_check(thread_t *td, ksiginfo_t *out) {
  proc_t *p = td->td_proc;

  assert(p != NULL);
  assert(mtx_owned(&p->p_lock));

  signo_t sig = sig_pending(td);
  if (sig == 0) {
    /* No pending signals, signal checking done. */
    WITH_SPIN_LOCK (td->td_lock)
      td->td_flags &= ~TDF_NEEDSIGCHK;
    return 0;
  }
  sigpend_get(&td->td_sigpend, sig, out);

  /* We should never get a pending signal that's ignored,
   * since we discard such signals in do_sigaction(). */
  assert(!sig_ignored(p->p_sigactions, sig));

  /* If we reached here, then the signal has to be posted. */
  return sig;
}

void sig_post(ksiginfo_t *ksi) {
  thread_t *td = thread_self();
  proc_t *p = proc_self();

  assert(p != NULL);
  assert(mtx_owned(&p->p_lock));
  assert(ksi != NULL);

  signo_t sig = ksi->ksi_signo;
  sigaction_t *sa = &p->p_sigactions[sig];
  sig_t handler = sa->sa_handler;

  assert(handler != SIG_IGN);

  /* Theoretically, we should hold all_proc_mtx since sig_ignore_ttystop()
   * reads pg_jobc, but the race here is harmless. */
  if (sig_ignore_ttystop(p, sig))
    return;

  if (handler == SIG_DFL && defact(sig) == SA_KILL) {
    /* Terminate this thread as result of a signal. */
    sig_exit(td, sig);
    __unreachable();
  }

  if (sa->sa_handler == SIG_DFL && defact(sig) == SA_STOP) {
    klog("Stopping thread %lu in process PID(%d)", td->td_tid, p->p_pid);
    p->p_stopsig = sig;
    p->p_state = PS_STOPPED;
    p->p_flags |= PF_STATE_CHANGED;
    WITH_PROC_LOCK(p->p_parent) {
      proc_wakeup_parent(p->p_parent);
      sig_child(p, CLD_STOPPED);
    }
    WITH_SPIN_LOCK (td->td_lock) { td->td_flags |= TDF_STOPPING; }
    proc_unlock(p);
    /* We're holding no locks here, so our process can be continued before we
     * actually stop the thread. This is why we need the TDF_STOPPING flag. */
    spin_lock(td->td_lock);
    if (td->td_flags & TDF_STOPPING) {
      td->td_flags &= ~TDF_STOPPING;
      td->td_state = TDS_STOPPED;
      sched_switch(); /* Releases td_lock. */
    } else {
      spin_unlock(td->td_lock);
    }
    proc_lock(p);
    return;
  }

  klog("Post signal %s (handler %p) to thread %lu in process PID(%d)",
       sig_name[sig], handler, td->td_tid, p->p_pid);

  sigset_t *return_mask;
  if (td->td_pflags & TDP_OLDSIGMASK) {
    return_mask = &td->td_oldsigmask;
    td->td_pflags &= ~TDP_OLDSIGMASK;
  } else {
    return_mask = &td->td_sigmask;
  }

  /* Normally the `sig_post` would have more to do, but our signal
   * implementation is very limited for now. All `sig_post` has to do is to
   * pass `sa` to platform-specific `sig_send`. */
  sig_send(sig, return_mask, sa, ksi);

  /* Set handler's signal mask. */
  __sigplusset(&sa->sa_mask, &td->td_sigmask);
  __sigaddset(&td->td_sigmask, sig);
  __sigminusset(&cantmask, &td->td_sigmask);
}

__noreturn void sig_exit(thread_t *td, signo_t sig) {
  klog("PID(%d) terminated due to signal %s ", td->td_proc->p_pid,
       sig_name[sig]);
  proc_exit(MAKE_STATUS_SIG_TERM(sig));
}

int do_sigreturn(void) {
  return sig_return();
}
