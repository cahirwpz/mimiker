#include <sys/thread.h>
#include <sys/context.h>
#include <machine/exc.h>

extern __noreturn void thread_exit(void);
extern __noreturn void kern_exc_leave(void);

void thread_entry_setup(thread_t *td, entry_fn_t target, void *arg) {
  kstack_t *stk = &td->td_kstack;

  kstack_reset(stk);

  /* For threads that are allowed to enter user-space (for now - all of them),
   * full exception frame has to be allocated at the bottom of kernel stack.
   * Just under it there's a kernel exception frame (cpu part of full one) that
   * is used to enter kernel thread for the first time. */
  exc_frame_t *uframe = kstack_alloc_s(stk, exc_frame_t);
  exc_frame_t *kframe = kstack_alloc_s(stk, cpu_exc_frame_t);
  ctx_t *kctx = kstack_alloc_s(stk, ctx_t);

  td->td_uframe = uframe;
  td->td_kframe = kframe;
  td->td_kctx = kctx;

  /* Initialize registers just for ctx_switch to work correctly. */
  ctx_init(kctx, kern_exc_leave, kframe);

  /* This is the context that kern_exc_leave will restore. */
  exc_frame_init(kframe, target, uframe, EF_KERNEL);
  exc_frame_setup_call(kframe, (register_t)thread_exit, (register_t)arg, 0);
}
