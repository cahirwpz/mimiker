#include <signal.h>
#include <thread.h>
#include <malloc.h>
#include <stdc.h>
#include <errno.h>
#include <sysent.h>
#include <systm.h>
#include <proc.h>

static MALLOC_DEFINE(sig_pool, "signal handlers pool", 2, 2);

static sighandler_t *signal_default_actions[SIG_LAST] = {
    [SIGINT] = SIG_TERM,  [SIGILL] = SIG_TERM,  [SIGFPE] = SIG_TERM,
    [SIGABRT] = SIG_TERM, [SIGSEGV] = SIG_TERM, [SIGKILL] = SIG_TERM,
    [SIGCHLD] = SIG_IGN,  [SIGUSR1] = SIG_TERM, [SIGUSR2] = SIG_TERM,
};

static const char *signal_names[SIG_LAST] = {
    [SIGINT] = "SIGINT",   [SIGILL] = "SIGILL",   [SIGABRT] = "SIGABRT",
    [SIGFPE] = "SIGFPE",   [SIGSEGV] = "SIGSEGV", [SIGKILL] = "SIGKILL",
    [SIGTERM] = "SIGTERM", [SIGCHLD] = "SIGCHLD", [SIGUSR1] = "SIGUSR1",
    [SIGUSR2] = "SIGUSR2",
};

sighand_t *sighand_new() {
  sighand_t *sh = kmalloc(sig_pool, sizeof(sighand_t), M_ZERO);
  mtx_init(&sh->sh_mtx, MTX_RECURSE);
  sh->sh_refcount = 1;
  return sh;
}
static void sighand_free(sighand_t *sh) {
  assert(sh->sh_refcount == 0);
  /* No need to lock the mutex, we have the last existing reference. */
  kfree(sig_pool, sh);
}

sighand_t *sighand_copy(sighand_t *sh) {
  sighand_t *new = sighand_new();
  mtx_scoped_lock(&sh->sh_mtx);
  memcpy(new->sh_actions, sh->sh_actions, sizeof(sigaction_t) * SIG_LAST);
  return new;
}

void sighand_ref(sighand_t *sh) {
  mtx_scoped_lock(&sh->sh_mtx);
  sh->sh_refcount++;
}
void sighand_unref(sighand_t *sh) {
  mtx_scoped_lock(&sh->sh_mtx);
  if (--sh->sh_refcount == 0)
    sighand_free(sh);
}

int do_kill(tid_t tid, int sig) {
  thread_t *target = thread_get_by_tid(tid);
  if (target == NULL)
    return -EINVAL;

  return signal(target, sig);
}

int do_sigaction(int sig, const sigaction_t *act, sigaction_t *oldact) {
  thread_t *td = thread_self();
  assert(td->td_proc);
  mtx_scoped_lock(&td->td_proc->p_lock);
  /* No need to acquire thread lock, we only look up a reference to the signal
   * handler structure. */
  sighand_t *sh = td->td_proc->p_sighand;

  if (sig < 0 || sig >= SIG_LAST)
    return -EINVAL;

  if (sig == SIGKILL)
    return -EINVAL;

  mtx_scoped_lock(&sh->sh_mtx);
  if (oldact != NULL)
    memcpy(oldact, &sh->sh_actions[sig], sizeof(sigaction_t));

  memcpy(&sh->sh_actions[sig], act, sizeof(sigaction_t));

  return 0;
}

static sighandler_t *get_sigact(int sig, proc_t *p) {
  /* Assume p->p_lock is already owned. */
  sighandler_t *h = p->p_sighand->sh_actions[sig].sa_handler;
  if (h == SIG_DFL)
    h = signal_default_actions[sig];
  return h;
}

int signal(thread_t *target, signo_t sig) {

  /* NOTE: This is a very simple implementation! Unimplemented features:
     - Thread tracing and debugging
     - Multiple threads sharing PID (aka: process)
     - Signal masks
     - SIGSTOP/SIGCONT
     These limitations (plus the fact that we currently have very little thread
     states) make the logic of posting a signal very simple!
  */
  assert(sig < SIG_LAST);
  assert(target->td_proc);

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
    int signo = SIG_LAST;
    bit_ffs(td->td_sigpend, SIG_LAST, &signo);
    if (signo < 0 || signo >= SIG_LAST)
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
      log("Thread %lu terminated due to signal %s!", td->td_tid,
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
  sigaction_t *sa = td->td_proc->p_sighand->sh_actions + sig;

  assert(sa->sa_handler != SIG_IGN && sa->sa_handler != SIG_DFL);

  log("Postsig for %s with handler %p", signal_names[sig], sa->sa_handler);
  /* Normally the postsig would have more to do, but our signal implemetnation
     is very limited for now, and all postsig has to do is to pass the sa to
     platform-specific sendsig. */

  platform_sendsig(sig, sa);
}

int do_sigreturn() {
  return platform_sigreturn();
}

int sys_kill(thread_t *td, syscall_args_t *args) {
  tid_t tid = args->args[0];
  signo_t sig = args->args[1];
  kprintf("[syscall] kill(%lu, %d)\n", tid, sig);

  return do_kill(tid, sig);
}

int sys_sigaction(thread_t *td, syscall_args_t *args) {
  int signo = args->args[0];
  char *p_newact = (char *)args->args[1];
  char *p_oldact = (char *)args->args[2];
  kprintf("[syscall] sigaction(%d, %p, %p)\n", signo, p_newact, p_oldact);

  sigaction_t newact;
  sigaction_t oldact;

  copyin(p_newact, &newact, sizeof(sigaction_t));

  int res = do_sigaction(signo, &newact, &oldact);
  if (res < 0)
    return res;

  if (p_oldact != NULL)
    copyout(&oldact, p_oldact, sizeof(sigaction_t));

  return res;
}

int sys_sigreturn(thread_t *td, syscall_args_t *args) {
  kprintf("[syscall] sigreturn()\n");
  return do_sigreturn();
}
