#include <stdc.h>
#include <context.h>
#include <thread.h>

extern noreturn void kernel_exit();
extern noreturn void kern_exc_leave();

void ctx_init(thread_t *td, void (*target)()) {
  register void *gp asm("$gp");

  void *sp = td->td_kstack.stk_base + td->td_kstack.stk_size;
  exc_frame_t *kframe = sp - sizeof(exc_frame_t);
  ctx_t *kctx = &td->td_kctx;

  bzero(kframe, sizeof(exc_frame_t));
  bzero(kctx, sizeof(ctx_t));

  td->td_kframe = kframe;

  reg_t sr = (reg_t)mips32_get_c0(C0_STATUS);

  /* Initialize registers just for ctx_boot to work correctly. */
  kctx->pc = (reg_t)kern_exc_leave;
  kctx->sp = (reg_t)kframe;
  kctx->tcb = (reg_t)td;

  /* This is the context that ctx_boot will restore. */
  kframe->pc = (reg_t)target;
  kframe->ra = (reg_t)kernel_exit;
  kframe->gp = (reg_t)gp;
  kframe->sp = (reg_t)sp;
  kframe->sr = (reg_t)sr;
}
