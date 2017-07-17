#define KL_LOG KL_SIGNAL
#include <klog.h>
#include <signal.h>
#include <thread.h>
#include <malloc.h>
#include <stdc.h>
#include <errno.h>
#include <sysent.h>
#include <proc.h>
#include <wait.h>

#define SA_IGNORE 0x01
#define SA_KILL 0x02

static int def_sigact[NSIG] = {
    [SIGINT] = SA_KILL,    [SIGILL] = SA_KILL,  [SIGFPE] = SA_KILL,
    [SIGABRT] = SA_KILL,   [SIGSEGV] = SA_KILL, [SIGKILL] = SA_KILL,
    [SIGCHLD] = SA_IGNORE, [SIGUSR1] = SA_KILL, [SIGUSR2] = SA_KILL,
};

static const char *sig_name[NSIG] = {
    [SIGINT] = "SIGINT",   [SIGILL] = "SIGILL",   [SIGABRT] = "SIGABRT",
    [SIGFPE] = "SIGFPE",   [SIGSEGV] = "SIGSEGV", [SIGKILL] = "SIGKILL",
    [SIGTERM] = "SIGTERM", [SIGCHLD] = "SIGCHLD", [SIGUSR1] = "SIGUSR1",
    [SIGUSR2] = "SIGUSR2",
};

int do_kill(pid_t pid, signo_t sig) {
  proc_t *target = proc_find(pid);
  if (target == NULL)
    return -EINVAL;
  return sig_send(target, sig);
}

int do_sigaction(signo_t sig, const sigaction_t *act, sigaction_t *oldact) {
  thread_t *td = thread_self();
  assert(td->td_proc);
  SCOPED_MTX_LOCK(&td->td_proc->p_lock);

  if (sig >= NSIG)
    return -EINVAL;

  if (sig == SIGKILL)
    return -EINVAL;

  if (oldact != NULL)
    memcpy(oldact, &td->td_proc->p_sigactions[sig], sizeof(sigaction_t));

  memcpy(&td->td_proc->p_sigactions[sig], act, sizeof(sigaction_t));

  return 0;
}

static int sig_default(signo_t sig) {
  assert(sig <= NSIG);
  return def_sigact[sig];
}

/*
 * NOTE: This is a very simple implementation! Unimplemented features:
 * - Thread tracing and debugging
 * - Multiple threads in a process
 * - Signal masks
 * - SIGSTOP/SIGCONT
 * These limitations (plus the fact that we currently have very little thread
 * states) make the logic of sending a signal very simple!
 */
int sig_send(proc_t *proc, signo_t sig) {
  assert(sig < NSIG);
  assert(proc->p_nthreads == 1);

  /* XXX: If we were to support multiple threads in a process, repeat everything
     below for each thread in proc. */
  thread_t *target = TAILQ_FIRST(&proc->p_threads);

  SCOPED_MTX_LOCK(&target->td_lock);

  /* If the thread is already dead, don't post a signal. */
  if (target->td_state == TDS_DEAD)
    return -EINVAL;

  WITH_MTX_LOCK (&target->td_proc->p_lock) {
    /* If the signal is ignored, don't even bother posting it. */
    sighandler_t *handler = target->td_proc->p_sigactions[sig].sa_handler;
    if (handler == SIG_IGN ||
        (sig_default(sig) == SA_IGNORE && handler == SIG_DFL))
      return 0;
  }

  bit_set(target->td_sigpend, sig);

  /* TODO: If a thread is sleeping interruptibly (!), wake it up, so that it
   * continues execution and the signal gets delivered soon.
   * However, we don't currently distinguish between two sleeping states.
   * So even if the signal is SIGKILL we cannot just wake up the thread
   * regardless of conditions. */
  sig_notify(target);

  return 0;
}

void sig_notify(thread_t *td) {
  assert(mtx_owned(&td->td_lock));
  /* TODO: Check if thread really has any pending unmasked signal. */
  td->td_flags |= TDF_NEEDSIGCHK;
}

int sig_check(thread_t *td) {
  assert(td->td_proc);

  signo_t sig = NSIG;
  while (true) {
    bit_ffs(td->td_sigpend, NSIG, &sig);
    if (sig >= NSIG)
      return 0; /* No pending signals. */

    bit_clear(td->td_sigpend, sig);

    SCOPED_MTX_LOCK(&td->td_proc->p_lock);
    sighandler_t *handler = td->td_proc->p_sigactions[sig].sa_handler;

    if (handler == SIG_IGN ||
        (handler == SIG_DFL && sig_default(sig) == SA_IGNORE))
      continue;

    /* Terminate this thread as result of a signal. */
    if (handler == SIG_DFL && sig_default(sig) == SA_KILL) {
      klog("Thread %lu terminated due to signal %s!", td->td_tid,
           sig_name[sig]);
      goto term;
    }

    /* If we reached here, then the signal has a custom handler. */
    return sig;
  }

term:
  /* Release the lock held by the parent. */
  mtx_unlock(&td->td_lock);
  proc_exit(MAKE_STATUS_SIG_TERM(sig));
  __unreachable();
}

void sig_deliver(signo_t sig) {
  thread_t *td = thread_self();
  assert(td->td_proc);
  SCOPED_MTX_LOCK(&td->td_proc->p_lock);
  sigaction_t *sa = td->td_proc->p_sigactions + sig;

  assert(sa->sa_handler != SIG_IGN && sa->sa_handler != SIG_DFL);

  klog("Deliver signal %s (handler %p) to %lu", sig_name[sig], sa->sa_handler,
       td->td_tid);

  /* Normally the postsig would have more to do, but our signal implementation
     is very limited for now, and all `sig_deliver` has to do is to pass `sa` to
     platform-specific `sig_deliver`. */
  platform_sig_deliver(sig, sa);
}

int do_sigreturn(void) {
  return platform_sig_return();
}
