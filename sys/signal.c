#define KL_LOG KL_SIGNAL
#include <klog.h>
#include <signal.h>
#include <thread.h>
#include <malloc.h>
#include <stdc.h>
#include <errno.h>
#include <sysent.h>
#include <proc.h>

static sighandler_t *def_sigact[NSIG] = {
    [SIGINT] = SIG_TERM,  [SIGILL] = SIG_TERM,  [SIGFPE] = SIG_TERM,
    [SIGABRT] = SIG_TERM, [SIGSEGV] = SIG_TERM, [SIGKILL] = SIG_TERM,
    [SIGCHLD] = SIG_IGN,  [SIGUSR1] = SIG_TERM, [SIGUSR2] = SIG_TERM,
};

static const char *sig_name[NSIG] = {
    [SIGINT] = "SIGINT",   [SIGILL] = "SIGILL",   [SIGABRT] = "SIGABRT",
    [SIGFPE] = "SIGFPE",   [SIGSEGV] = "SIGSEGV", [SIGKILL] = "SIGKILL",
    [SIGTERM] = "SIGTERM", [SIGCHLD] = "SIGCHLD", [SIGUSR1] = "SIGUSR1",
    [SIGUSR2] = "SIGUSR2",
};

int do_kill(pid_t pid, int sig) {
  proc_t *target = proc_find(pid);
  if (target == NULL)
    return -EINVAL;
  return sig_send(target, sig);
}

int do_sigaction(int sig, const sigaction_t *act, sigaction_t *oldact) {
  thread_t *td = thread_self();
  assert(td->td_proc);
  mtx_scoped_lock(&td->td_proc->p_lock);

  if (sig < 0 || sig >= NSIG)
    return -EINVAL;

  if (sig == SIGKILL)
    return -EINVAL;

  if (oldact != NULL)
    memcpy(oldact, &td->td_proc->p_sigactions[sig], sizeof(sigaction_t));

  memcpy(&td->td_proc->p_sigactions[sig], act, sizeof(sigaction_t));

  return 0;
}

static sighandler_t *get_sigact(int sig, proc_t *p) {
  assert(mtx_owned(&p->p_lock));
  sighandler_t *handler = p->p_sigactions[sig].sa_handler;
  return (handler == SIG_DFL) ? def_sigact[sig] : handler;
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

  mtx_scoped_lock(&target->td_lock);

  /* If the thread is already dead, don't post a signal. */
  if (target->td_state == TDS_INACTIVE)
    return -EINVAL;

  mtx_lock(&target->td_proc->p_lock);
  /* If the signal is ignored, don't even bother posting it. */
  if (get_sigact(sig, target->td_proc) == SIG_IGN)
    return 0;
  mtx_unlock(&target->td_proc->p_lock);

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

  while (true) {
    int signo = NSIG;
    bit_ffs(td->td_sigpend, NSIG, &signo);
    if (signo < 0 || signo >= NSIG)
      return 0; /* No pending signals. */

    bit_clear(td->td_sigpend, signo);

    mtx_scoped_lock(&td->td_proc->p_lock);
    sighandler_t *handler = get_sigact(signo, td->td_proc);
    assert(handler != SIG_DFL);

    if (handler == SIG_IGN)
      continue;

    /* Terminate this thread as result of a signal. */
    if (handler == SIG_TERM) {
      klog("Thread %lu terminated due to signal %s!", td->td_tid,
           sig_name[signo]);
      goto term;
    }

    /* If we reached here, then the signal has a custom handler. */
    return signo;
  }

term:
  /* Release the lock held by the parent. */
  mtx_unlock(&td->td_lock);
  thread_exit(-1);
  __unreachable();
}

void sig_deliver(int sig) {
  thread_t *td = thread_self();
  assert(td->td_proc);
  mtx_scoped_lock(&td->td_proc->p_lock);
  sigaction_t *sa = td->td_proc->p_sigactions + sig;

  assert(sa->sa_handler != SIG_IGN && sa->sa_handler != SIG_DFL);

  klog("Deliver signal %s (handler %p) to %lu", sig_name[sig], sa->sa_handler,
       td->td_tid);

  /* Normally the postsig would have more to do, but our signal implementation
     is very limited for now, and all `sig_deliver` has to do is to pass `sa` to
     platform-specific `sig_deliver`. */
  platform_sig_deliver(sig, sa);
}

int do_sigreturn() {
  return platform_sig_return();
}
