#include <sys/thread.h>
#include <sys/context.h>
#include <mips/mcontext.h>

extern __noreturn void thread_exit(void);
extern __noreturn void kern_exc_leave(void);

void thread_entry_setup(thread_t *td, entry_fn_t target, void *arg) {
  kstack_t *stk = &td->td_kstack;

  kstack_reset(stk);

  /* For threads that are allowed to enter user-space (for now - all of them),
   * full exception frame has to be allocated at the bottom of kernel stack.
   * Just under it there's a kernel exception frame (cpu part of full one) that
   * is used to enter kernel thread for the first time. */
  mcontext_t *uctx = kstack_alloc_s(stk, mcontext_t);
  ctx_t *kframe = kstack_alloc_s(stk, ctx_t);
  ctx_t *kctx = kstack_alloc_s(stk, ctx_t);

  td->td_uctx = uctx;
  td->td_kframe = NULL;
  td->td_kctx = kctx;

  /* Initialize registers in order to switch to kframe context. */
  ctx_init(kctx, kern_exc_leave, kframe);

  /* This is the context that kern_exc_leave will restore. */
  ctx_init(kframe, target, uctx);
  ctx_setup_call(kframe, (register_t)thread_exit, (register_t)arg);
}
