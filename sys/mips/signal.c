#define KL_LOG KL_SIGNAL
#include <sys/mimiker.h>
#include <sys/signal.h>
#include <sys/thread.h>
#include <sys/klog.h>
#include <mips/context.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/ucontext.h>

typedef struct sig_ctx {
  ucontext_t sc_uc;
  siginfo_t sc_info;
  /* TODO: Store handler signal mask. */
  /* TODO: Store previous stack data, if the sigaction requested a different
   * stack. */
} sig_ctx_t;

static void stack_unusable(thread_t *td, register_t sp) {
  /* This thread has a corrupted stack, it can no longer react on a signal with
   * a custom handler. Kill the process. */
  klog("User stack (%p) is corrupted, terminating with SIGILL!", sp);
  sig_exit(td, SIGILL);
  __unreachable();
}

int sig_send(signo_t sig, sigset_t *mask, sigaction_t *sa, ksiginfo_t *ksi) {
  thread_t *td = thread_self();

  user_ctx_t *uctx = td->td_uctx;

  /* Prepare signal context. */
  sig_ctx_t ksc = {.sc_info = ksi->ksi_info};
  user_ctx_copy(&ksc.sc_uc.uc_mcontext, uctx);
  ksc.sc_uc.uc_sigmask = *mask;

  /* Copyout sigcode to user stack. */
  unsigned sigcode_size = esigcode - sigcode;
  void *sp = (void *)_REG(uctx, SP) - sigcode_size;
  void *sigcode_stack_addr = sp;

  if (copyout(sigcode, sigcode_stack_addr, sigcode_size))
    stack_unusable(td, _REG(uctx, SP));

  /* Copyout signal context to user stack. */
  sig_ctx_t *cp = (sig_ctx_t *)sp;
  cp--;
  if (copyout(&ksc, cp, sizeof(sig_ctx_t)))
    stack_unusable(td, _REG(uctx, SP));

  /* Prepare user context so that on return to usermode the handler gets
   * executed. No need to check whether the handler address is valid (aligned,
   * user space, mapped memory, executable). If it is not, an exception will be
   * raised and the user process will get the punishment it deserves (SIGILL,
   * SIGSEGV). */
  _REG(uctx, EPC) = (register_t)sa->sa_handler;
  /* Set arguments to signal number, signal info, and user context. */
  _REG(uctx, A0) = sig;
  _REG(uctx, A1) = (register_t)&cp->sc_info;
  _REG(uctx, A2) = (register_t)&cp->sc_uc.uc_mcontext;
  /* The calling convention is such that the callee may write to the address
   * pointed by sp before extending the stack - so we need to set it 1 word
   * before the stored context! */
  _REG(uctx, SP) = (register_t)((intptr_t *)cp - 1);
  /* Also, make sure that sigcode runs when the handler exits. */
  _REG(uctx, RA) = (register_t)sigcode_stack_addr;

  return 0;
}

int sig_return(ucontext_t *ucp) {
  int error = 0;
  thread_t *td = thread_self();
  ucontext_t uc;

  user_ctx_t *uctx = td->td_uctx;

  error = copyin_s(ucp, uc);
  if (error)
    return error;

  /* Restore user context. */
  user_ctx_copy(uctx, &uc.uc_mcontext);

  WITH_MTX_LOCK (&td->td_proc->p_lock)
    error = do_sigprocmask(SIG_SETMASK, &uc.uc_sigmask, NULL);
  assert(error == 0);

  return EJUSTRETURN;
}

void sig_trap(ctx_t *ctx, signo_t sig) {
  proc_t *proc = proc_self();
  WITH_MTX_LOCK (&proc->p_lock)
    sig_kill(proc, &DEF_KSI_TRAP(sig));
}
