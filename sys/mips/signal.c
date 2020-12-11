#define KL_LOG KL_SIGNAL
#include <sys/mimiker.h>
#include <sys/signal.h>
#include <sys/thread.h>
#include <sys/klog.h>
#include <mips/context.h>
#include <sys/errno.h>
#include <sys/proc.h>

#define SIG_CTX_MAGIC 0xDACBAEE3

typedef struct sig_ctx {
  uint32_t magic; /* Integrity control. */
  siginfo_t info;
  user_ctx_t uctx;
  sigset_t mask; /* signal mask to restore in sigreturn() */
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
  sig_ctx_t ksc = {
    .magic = SIG_CTX_MAGIC, .info = ksi->ksi_info, .mask = *mask};
  user_ctx_copy(&ksc.uctx, uctx);

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
  _REG(uctx, A1) = (register_t)&cp->info;
  _REG(uctx, A2) = (register_t)&cp->uctx;
  /* The calling convention is such that the callee may write to the address
   * pointed by sp before extending the stack - so we need to set it 1 word
   * before the stored context! */
  _REG(uctx, SP) = (register_t)((intptr_t *)cp - 1);
  /* Also, make sure that sigcode runs when the handler exits. */
  _REG(uctx, RA) = (register_t)sigcode_stack_addr;

  return 0;
}

int sig_return(void) {
  int error = 0;
  thread_t *td = thread_self();
  sig_ctx_t ksc;

  user_ctx_t *uctx = td->td_uctx;
  /* TODO: We assume the stored user context is where user stack is. This
   * usually works, but the signal handler may switch the stack, or perform an
   * arbitrary jump. It may also call sigreturn() when its stack is not empty
   * (although it should not). Normally the C library tracks the location
   * where the context was stored: it remembers the stack pointer at the point
   * when the handler (or a wrapper!) was called, and passes that location
   * back to us as an argument to sigreturn (which is, again, provided to the
   * user program with a wrapper). We don't do any of that fancy stuff yet,
   * but when we do, the following will need to get the scp pointer address
   * from a syscall argument. */
  sig_ctx_t *scp = (sig_ctx_t *)((intptr_t *)_REG(uctx, SP) + 1);
  error = copyin_s(scp, ksc);
  if (error)
    return error;
  if (ksc.magic != SIG_CTX_MAGIC) {
    klog("User context at %p corrupted!", scp);
    sig_trap((ctx_t *)uctx, SIGILL);
    return EINVAL;
  }

  /* Restore user context. */
  user_ctx_copy(uctx, &ksc.uctx);

  WITH_MTX_LOCK (&td->td_proc->p_lock)
    error = do_sigprocmask(SIG_SETMASK, &ksc.mask, NULL);
  assert(error == 0);

  return EJUSTRETURN;
}

void sig_trap(ctx_t *ctx, signo_t sig) {
  proc_t *proc = proc_self();
  WITH_MTX_LOCK (&proc->p_lock)
    sig_kill(proc, &DEF_KSI_TRAP(sig));
}
