#define KL_LOG KL_SIGNAL
#include <klog.h>
#include <signal.h>
#include <thread.h>
#include <malloc.h>
#include <stdc.h>
#include <errno.h>
#include <sysent.h>
#include <proc.h>

static sighandler_t *signal_default_actions[NSIG] = {
    [SIGINT] = SIG_TERM,  [SIGILL] = SIG_TERM,  [SIGFPE] = SIG_TERM,
    [SIGABRT] = SIG_TERM, [SIGSEGV] = SIG_TERM, [SIGKILL] = SIG_TERM,
    [SIGCHLD] = SIG_IGN,  [SIGUSR1] = SIG_TERM, [SIGUSR2] = SIG_TERM,
};

static const char *signal_names[NSIG] = {
    [SIGINT] = "SIGINT",   [SIGILL] = "SIGILL",   [SIGABRT] = "SIGABRT",
    [SIGFPE] = "SIGFPE",   [SIGSEGV] = "SIGSEGV", [SIGKILL] = "SIGKILL",
    [SIGTERM] = "SIGTERM", [SIGCHLD] = "SIGCHLD", [SIGUSR1] = "SIGUSR1",
    [SIGUSR2] = "SIGUSR2",
};

int do_kill(pid_t pid, int sig) {
  proc_t *target = proc_find(pid);
  if (target == NULL)
    return -EINVAL;

  return signal(target, sig);
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
  sighandler_t *h = p->p_sigactions[sig].sa_handler;
  if (h == SIG_DFL)
    h = signal_default_actions[sig];
  return h;
}

int signal(proc_t *proc, signo_t sig) {

  /* NOTE: This is a very simple implementation! Unimplemented features:
     - Thread tracing and debugging
     - Multiple threads in a process
     - Signal masks
     - SIGSTOP/SIGCONT
     These limitations (plus the fact that we currently have very little thread
     states) make the logic of posting a signal very simple!
  */
  assert(sig < NSIG);
  assert(proc->p_nthreads == 1);

  /* XXX: If we were to support multiple threads in a process, repeat everything
     below for each thread in proc. */
  thread_t *target = TAILQ_FIRST(&proc->p_threads);

  mtx_scoped_lock(&target->td_lock);

  if (target->td_state == TDS_INACTIVE) {
    /* The thread is already dead. */
    return -EINVAL;
  }

  /* If the signal is ignored, don't even bother posting it. */
  if (get_sigact(sig, target->td_proc) == SIG_IGN)
    return 0;

  bit_set(target->td_sigpend, sig);

  /* TODO: If a thread is sleeping, wake it up, so that it continues execution
     and the signal gets delivered soon. However, we don't currently distinguish
     between user-mode sleep and kernel-mode waiting. */

  /* If the signal is KILL, wake up the thread regardless of conditions, so that
     it terminates ASAP. */
  if (sig == SIGKILL && target->td_state == TDS_WAITING) {
    target->td_state = TDS_READY;
  }

  signotify(target);

  return 0;
}

void signotify(thread_t *td) {
  assert(mtx_owned(&td->td_lock));
  /* TODO: Check if thread really has any pending unmasked signal. */
  td->td_flags |= TDF_NEEDSIGCHK;
}

int issignal(thread_t *td) {
  assert(td->td_proc);
  while (true) {
    int signo = NSIG;
    bit_ffs(td->td_sigpend, NSIG, &signo);
    if (signo < 0 || signo >= NSIG)
      return 0; /* No pending signals. */

    bit_clear(td->td_sigpend, signo);

    mtx_scoped_lock(&td->td_proc->p_lock);
    sighandler_t *h = get_sigact(signo, td->td_proc);
    assert(h != SIG_DFL);
    if (h == SIG_IGN)
      continue;

    if (h == SIG_TERM) {
      /* Terminate this thread as result of a signal. */

      /* Diagnostic message */
      klog("Thread %lu terminated due to signal %s!", td->td_tid,
           signal_names[signo]);

      /* Release the lock held by the parent. */
      mtx_unlock(&td->td_lock);
      /* Release the lock hold by scoped_lock */
      mtx_unlock(&td->td_proc->p_lock);
      thread_exit(-1);
      __builtin_unreachable();
    }

    /* If we reached here, then the signal has a custom handler. */
    return signo;
  }
}

extern int platform_sendsig(int sig, sigaction_t *sa);
extern int platform_sigreturn();

void postsig(int sig) {
  thread_t *td = thread_self();
  assert(td->td_proc);
  mtx_scoped_lock(&td->td_proc->p_lock);
  sigaction_t *sa = td->td_proc->p_sigactions + sig;

  assert(sa->sa_handler != SIG_IGN && sa->sa_handler != SIG_DFL);

  klog("Postsig for %s with handler %p", signal_names[sig], sa->sa_handler);
  /* Normally the postsig would have more to do, but our signal implemetnation
     is very limited for now, and all postsig has to do is to pass the sa to
     platform-specific sendsig. */

  platform_sendsig(sig, sa);
}

int do_sigreturn() {
  return platform_sigreturn();
}

