#include <stdc.h>
#include <context.h>
#include <thread.h>
#include <mips/exc.h>

extern noreturn void thread_exit(void);
extern noreturn void kern_exc_leave(void);

void ctx_init(thread_t *td, void (*target)(void *), void *arg) {
  register void *gp asm("$gp");

  void *sp = td->td_kstack.stk_base + td->td_kstack.stk_size;
  exc_frame_t *kframe = sp - sizeof(exc_frame_t);
  ctx_t *kctx = &td->td_kctx;

  bzero(kframe, sizeof(exc_frame_t));
  bzero(kctx, sizeof(ctx_t));

  td->td_kframe = kframe;

  reg_t sr = (reg_t)mips32_get_c0(C0_STATUS);

  /* Initialize registers just for ctx_switch to work correctly. */
  kctx->pc = (reg_t)kern_exc_leave;
  kctx->sr = (reg_t)sr;
  kctx->sp = (reg_t)kframe;

  /* This is the context that kern_exc_leave will restore. */
  kframe->pc = (reg_t)target;
  kframe->ra = (reg_t)thread_exit;
  kframe->gp = (reg_t)gp;
  kframe->sp = (reg_t)sp;
  kframe->a0 = (reg_t)arg;

  /* Status register: Take interrupt mask and exception vector location from
     caller's context. */
  kframe->sr = sr & (SR_IMASK | SR_BEV);
  /* Set Interrupt Enable and Exception Level. */
  kframe->sr |= SR_EXL | SR_IE;
}

void uctx_init(thread_t *td, vm_addr_t pc, vm_addr_t sp) {
  bzero(&td->td_uctx, sizeof(exc_frame_t));

  td->td_uctx.gp = 0; /* Explicit. */
  td->td_uctx.pc = pc;
  td->td_uctx.sp = sp;
  td->td_uctx.ra = 0; /* Explicit. */
}

void exc_frame_set_retval(exc_frame_t *frame, reg_t value) {
  frame->v0 = value;
  frame->pc += 4;
}
