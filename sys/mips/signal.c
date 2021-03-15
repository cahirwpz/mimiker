#define KL_LOG KL_SIGNAL
#include <sys/mimiker.h>
#include <sys/signal.h>
#include <sys/thread.h>
#include <sys/klog.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/ucontext.h>

static register_t sig_stack_push(mcontext_t *uctx, void *data, size_t len) {
  _REG(uctx, SP) -= roundup(len, STACK_ALIGN);
  register_t sp = _REG(uctx, SP);
  if (copyout(data, (void *)sp, len)) {
    /* This thread has a corrupted stack, it can no longer react on a signal
     * with a custom handler. Kill the process. */
    klog("User stack (%p) is corrupted, terminating with SIGILL!", sp);
    sig_exit(thread_self(), SIGILL);
  }
  return sp;
}

int sig_send(signo_t sig, sigset_t *mask, sigaction_t *sa, ksiginfo_t *ksi) {
  thread_t *td = thread_self();
  mcontext_t *uctx = td->td_uctx;

  ucontext_t uc;
  mcontext_copy(&uc.uc_mcontext, uctx);
  uc.uc_sigmask = *mask;

  register_t sc_code = sig_stack_push(uctx, sigcode, esigcode - sigcode);
  register_t sc_info = sig_stack_push(uctx, ksi, sizeof(ksiginfo_t));
  register_t sc_uctx = sig_stack_push(uctx, &uc, sizeof(ucontext_t));

  /* Prepare user context so that on return to usermode the handler gets
   * executed. No need to check whether the handler address is valid (aligned,
   * user space, mapped memory, executable). If it is not, an exception will be
   * raised and the user process will get the punishment it deserves (SIGILL,
   * SIGSEGV). */
  _REG(uctx, EPC) = (register_t)sa->sa_handler;
  /* Set arguments to signal number, signal info, and user context. */
  _REG(uctx, A0) = sig;
  _REG(uctx, A1) = sc_info;
  _REG(uctx, A2) = sc_uctx;
  /* The calling convention is such that the callee may write to the address
   * pointed by sp before extending the stack - so we need to set it at least
   * 1 word before the stored context! */
  _REG(uctx, SP) -= STACK_ALIGN;
  /* Also, make sure that sigcode runs when the handler exits. */
  _REG(uctx, RA) = sc_code;

  return 0;
}

/* TODO: fill in various fields of ksiginfo_t based on context values. */
void sig_trap(ctx_t *ctx, signo_t sig) {
  proc_t *proc = proc_self();
  WITH_MTX_LOCK (&proc->p_lock)
    sig_kill(proc, &DEF_KSI_TRAP(sig));
}
