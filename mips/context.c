#include <stdc.h>
#include <context.h>
#include <thread.h>
#include <mips/exc.h>

extern noreturn void thread_exit(void);
extern noreturn void kern_exc_leave(void);

void ctx_init(thread_t *td, void (*target)(void *), void *arg) {
  register void *gp asm("$gp");

  stack_t *stk = &td->td_kstack;
  void *sp = stk->stk_base + stk->stk_size - sizeof(exc_frame_t);
  /* For threads that are allowed to enter user-space (for now - all of them),
   * full exception frame has to be allocated at the top of kernel stack.
   * Just under it there's a kernel exception frame (cpu part of full one) that
   * is used to enter kernel thread for the first time. */
  exc_frame_t *kframe = sp - sizeof(cpu_exc_frame_t);
  ctx_t *kctx = &td->td_kctx;

  bzero(kframe, sizeof(cpu_exc_frame_t));
  bzero(kctx, sizeof(ctx_t));

  td->td_uframe = sp;
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

void exc_frame_init(exc_frame_t *frame, vm_addr_t pc, vm_addr_t sp) {
  bzero(frame, sizeof(exc_frame_t));

  frame->gp = 0; /* Explicit. */
  frame->pc = (reg_t)pc;
  frame->sp = (reg_t)sp;
  frame->ra = 0; /* Explicit. */
}

void exc_frame_copy(exc_frame_t *to, exc_frame_t *from) {
  memcpy(to, from, sizeof(exc_frame_t));
}

void exc_frame_set_retval(exc_frame_t *frame, long value) {
  frame->v0 = (reg_t)value;
  frame->pc += 4;
}
