#define KL_LOG KL_SIGNAL
#include <signal.h>
#include <thread.h>
#include <systm.h>
#include <klog.h>
#include <errno.h>

#define SIG_CTX_MAGIC 0xDACBAEE3

typedef struct sig_ctx {
  uint32_t magic; /* Integrity control. */
  exc_frame_t ctx;
  fpu_ctx_t ctx_fpu;
  /* TODO: Store signal mask. */
  /* TODO: Store previous stack data, if the sigaction requested a different
   * stack. */
} sig_ctx_t;

/* Delivers a signal to user process. */
int platform_sendsig(int sig, sigaction_t *sa) {
  thread_t *td = thread_self();
  assert(mtx_owned(&td->td_lock));

  /* Prepare signal context. */
  sig_ctx_t ksc = {
    .magic = SIG_CTX_MAGIC, .ctx = td->td_uctx, .ctx_fpu = td->td_uctx_fpu};

  /* Copyout signal context to user stack. */
  sig_ctx_t *scp = (sig_ctx_t *)td->td_uctx.sp;
  scp--;

  int error = copyout(&ksc, scp, sizeof(sig_ctx_t));
  if (error) {
    /* This thread has a corrupted stack, we can no longer send any signals to
       it. Forcefully terminate the thread. TODO: Maybe sending a SIGKILL is
       safe here? */
    klog("Thread %lu is unable to receive a signal, terminating it.",
         td->td_tid);
    thread_exit(-1);
    __builtin_unreachable();
  }

  /* Prepare user context so that on return to usermode the handler gets
   * executed. */
  td->td_uctx.pc = (reg_t)sa->sa_handler;
  td->td_uctx.a0 = sig;
  /* The calling convention is such that the callee may write to the address
     pointed by sp before extending the stack - so we need to set it 1 word
     before the stored context! */
  td->td_uctx.sp = (reg_t)((uint32_t *)scp - 1);
  /* Also, make sure the restorer runs when the handler exits. */
  td->td_uctx.ra = (reg_t)sa->sa_restorer;

  return 0;
}

int platform_sigreturn() {
  thread_t *td = thread_self();
  mtx_scoped_lock(&td->td_lock);
  sig_ctx_t ksc;
  /* TODO: We assume the stored user context is where user stack is. This
     usually works, but the signal handler may switch the stack, or perform an
     arbitrary jump. It may also call sigreturn() when its stack is not empty
     (although it should not). Normally the C library tracks the location where
     the context was stored: it remembers the stack pointer at the point when
     the handler (or a wrapper!) was called, and passes that location back to us
     as an argument to sigreturn (which is, again, provided to the user program
     with a wrapper). We don't do any of that fancy stuff yet, but when we do,
     the following will need to get the scp pointer address from a syscall
     argument. */
  sig_ctx_t *scp = (sig_ctx_t *)((uint32_t *)td->td_uctx.sp + 1);
  int error = copyin(scp, &ksc, sizeof(sig_ctx_t));
  if (error)
    return error;
  if (ksc.magic != SIG_CTX_MAGIC) {
    return -EINVAL;
  }

  /* Restore user context. */
  td->td_uctx = ksc.ctx;
  td->td_uctx_fpu = ksc.ctx_fpu;

  return -EJUSTRETURN;
}
