#define KL_LOG KL_SIGNAL
#include <sys/mimiker.h>
#include <sys/signal.h>
#include <sys/thread.h>
#include <sys/klog.h>
#include <aarch64/context.h>
#include <sys/errno.h>
#include <sys/proc.h>

#define SIG_CTX_MAGIC 0xDACBAEE3

typedef struct sig_ctx {
  uint32_t magic; /* Integrity control. */
  siginfo_t info;
  user_ctx_t uctx;
  sigset_t mask; /* signal mask to restore in sigreturn() */
} sig_ctx_t;

static void stack_unusable(thread_t *td, register_t sp) {
  klog("User stack (%p) is corrupted, terminating with SIGILL!", sp);
  sig_exit(td, SIGILL);
  __unreachable();
}

int sig_send(signo_t sig, sigset_t *mask, sigaction_t *sa, ksiginfo_t *ksi) {
  thread_t *td = thread_self();
  user_ctx_t *uctx = td->td_uctx;

  sig_ctx_t ksc = {
    .magic = SIG_CTX_MAGIC, .info = ksi->ksi_info, .mask = *mask};
  user_ctx_copy(&ksc.uctx, uctx);

  size_t sigcode_size = esigcode - sigcode;
  void *sp = (void *)_REG(uctx, SP) - sigcode_size;
  void *sigcode_sp = sp;

  if (copyout(sigcode, sigcode_sp, sigcode_size))
    stack_unusable(td, _REG(uctx, SP));

  sig_ctx_t *cp = (sig_ctx_t *)sp;
  cp--;
  if (copyout(&ksc, cp, sizeof(sig_ctx_t)))
    stack_unusable(td, _REG(uctx, SP));

  _REG(uctx, PC) = (register_t)sa->sa_handler;
  _REG(uctx, X0) = sig;
  _REG(uctx, X1) = (register_t)&cp->info;
  _REG(uctx, X2) = (register_t)&cp->uctx;

  _REG(uctx, SP) = (register_t)((uintptr_t *)cp - 1);
  _REG(uctx, LR) = (register_t)sigcode_sp;

  return 0;
}

int sig_return(void) {
  int error = 0;
  thread_t *td = thread_self();
  sig_ctx_t ksc;

  user_ctx_t *uctx = td->td_uctx;
  sig_ctx_t *scp = (sig_ctx_t *)((uintptr_t *)_REG(uctx, SP) + 1);
  error = copyin_s(scp, ksc);
  if (error)
    return error;

  if (ksc.magic != SIG_CTX_MAGIC) {
    klog("User context at %p corrupted!", scp);
    sig_trap((ctx_t *)uctx, SIGILL);
    return EINVAL;
  }

  user_ctx_copy(uctx, &ksc.uctx);

  WITH_MTX_LOCK (&td->td_proc->p_lock) {
    error = do_sigprocmask(SIG_SETMASK, &ksc.mask, NULL);
  }
  assert(error == 0);

  return EJUSTRETURN;
}

void sig_trap(ctx_t *ctx, signo_t sig) {
  proc_t *proc = proc_self();
  WITH_MTX_LOCK (&proc->p_lock)
    sig_kill(proc, &DEF_KSI_TRAP(sig));
}
