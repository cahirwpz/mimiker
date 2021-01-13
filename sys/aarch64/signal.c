#define KL_LOG KL_SIGNAL
#include <sys/mimiker.h>
#include <sys/signal.h>
#include <sys/thread.h>
#include <sys/klog.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/ucontext.h>

typedef struct sig_ctx {
  ucontext_t sc_uc;
  siginfo_t sc_info;
} sig_ctx_t;

static void stack_unusable(thread_t *td, register_t sp) {
  klog("User stack (%p) is corrupted, terminating with SIGILL!", sp);
  sig_exit(td, SIGILL);
  __unreachable();
}

int sig_send(signo_t sig, sigset_t *mask, sigaction_t *sa, ksiginfo_t *ksi) {
  thread_t *td = thread_self();
  mcontext_t *uctx = td->td_uctx;

  /* Copyout sigcode to user stack. */
  unsigned sigcode_size = roundup(esigcode - sigcode, 16);
  void *sp = (void *)_REG(uctx, SP);
  sp -= sigcode_size;
  void *sigcode_ptr = sp;

  if (copyout(sigcode, sigcode_ptr, sigcode_size))
    stack_unusable(td, _REG(uctx, SP));

  /* Prepare signal context and copy it to user stack. */
  sig_ctx_t ksc = {.sc_info = ksi->ksi_info};
  mcontext_copy(&ksc.sc_uc.uc_mcontext, uctx);
  ksc.sc_uc.uc_sigmask = *mask;
  sp -= roundup(sizeof(sig_ctx_t), 16);
  sig_ctx_t *cp = sp;
  if (copyout(&ksc, cp, sizeof(sig_ctx_t)))
    stack_unusable(td, _REG(uctx, SP));

  _REG(uctx, ELR) = (register_t)sa->sa_handler;
  _REG(uctx, X0) = sig;
  _REG(uctx, X1) = (register_t)&cp->sc_info;
  _REG(uctx, X2) = (register_t)&cp->sc_uc.uc_mcontext;

  _REG(uctx, SP) = (register_t)(sp - sizeof(intptr_t *));
  _REG(uctx, LR) = (register_t)sigcode_ptr;

  return 0;
}

/* TODO: fill in various fields of ksiginfo_t based on context values. */
void sig_trap(ctx_t *ctx, signo_t sig) {
  proc_t *proc = proc_self();
  WITH_MTX_LOCK (&proc->p_lock)
    sig_kill(proc, &DEF_KSI_TRAP(sig));
}
