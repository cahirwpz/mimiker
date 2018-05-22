#define KL_LOG KL_SIGNAL
#include <signal.h>
#include <thread.h>
#include <systm.h>
#include <klog.h>
#include <mips/exc.h>
#include <errno.h>

#define SIG_CTX_MAGIC 0xDACBAEE3

typedef struct sig_ctx {
  uint32_t magic; /* Integrity control. */
  exc_frame_t frame;
  /* TODO: Store signal mask. */
  /* TODO: Store previous stack data, if the sigaction requested a different
   * stack. */
} sig_ctx_t;

/* Delivers a signal to user process. */
int platform_sig_deliver(signo_t sig, sigaction_t *sa) {
  thread_t *td = thread_self();
  assert(mtx_owned(&td->td_lock));

  exc_frame_t *uframe = td->td_uframe;

  /* Prepare signal context. */
  sig_ctx_t ksc = {.magic = SIG_CTX_MAGIC};
  exc_frame_copy(&ksc.frame, uframe);

  /* Copyout signal context to user stack. */
  sig_ctx_t *scp = (sig_ctx_t *)uframe->sp;
  scp--;

  int error = copyout(&ksc, scp, sizeof(sig_ctx_t));
  if (error) {
    /* This thread has a corrupted stack, it can no longer react on a signal
       with a custom handler. Kill the process. */
    klog("Thread %lu is unable to receive a signal, killing its process.",
         td->td_tid);
    sig_send(td->td_proc, SIGKILL);
    __unreachable();
  }

  /* Prepare user context so that on return to usermode the handler gets
   * executed. No need to check whether the handler address is valid (aligned,
   * user space, mapped memory, executable). If it is not, an exception will be
   * raised and the user process will get the punishment it deserves (SIGILL,
   * SIGSEGV). */
  uframe->pc = (reg_t)sa->sa_handler;
  uframe->a0 = sig;
  /* The calling convention is such that the callee may write to the address
     pointed by sp before extending the stack - so we need to set it 1 word
     before the stored context! */
  uframe->sp = (reg_t)((intptr_t *)scp - 1);
  /* Also, make sure the restorer runs when the handler exits. */
  uframe->ra = (reg_t)sa->sa_restorer;

  return 0;
}

int platform_sig_return(void) {
  thread_t *td = thread_self();
  SCOPED_MTX_LOCK(&td->td_lock);
  sig_ctx_t ksc;
  exc_frame_t *uframe = td->td_uframe;
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
  sig_ctx_t *scp = (sig_ctx_t *)((intptr_t *)uframe->sp + 1);
  int error = copyin_s(scp, ksc);
  if (error)
    return error;
  if (ksc.magic != SIG_CTX_MAGIC)
    return -EINVAL;

  /* Restore user context. */
  exc_frame_copy(uframe, &ksc.frame);

  return -EJUSTRETURN;
}
