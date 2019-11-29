#include <sys/libkern.h>
#include <sys/context.h>
#include <sys/exception.h>
#include <sys/thread.h>
#include <sys/errno.h>
#include <sys/ucontext.h>
#include <mips/ctx.h>
#include <mips/exc.h>

void ctx_init(ctx_t *ctx, void *pc, void *sp) {
  bzero(ctx, sizeof(ctx_t));

  ctx->pc = (register_t)pc;
  ctx->sp = (register_t)sp;
  /* Take SR from caller's context and enable interrupts. */
  ctx->sr = (register_t)mips32_get_c0(C0_STATUS) | SR_IE;
}

void ctx_set_retval(ctx_t *ctx, long value) {
  ctx->v0 = (register_t)value;
}

extern uint8_t _gp[]; /* Symbol provided by the linker. */

void exc_frame_init(exc_frame_t *frame, void *pc, void *sp, unsigned flags) {
  bool usermode = flags & EF_USER;

  bzero(frame, usermode ? sizeof(exc_frame_t) : sizeof(cpu_exc_frame_t));

  frame->gp = usermode ? 0 : (register_t)_gp;
  frame->pc = (register_t)pc;
  frame->sp = (register_t)sp;

  /* For user-mode exception frame we must make sure that:
   * - user mode is active,
   * - interrupts are enabled.
   * The rest will be covered by usr_exc_leave. */
  frame->sr = mips32_get_c0(C0_STATUS) | SR_IE | (usermode ? SR_KSU_USER : 0);
}

void exc_frame_copy(exc_frame_t *to, exc_frame_t *from) {
  memcpy(to, from, sizeof(exc_frame_t));
}

void exc_frame_setup_call(exc_frame_t *frame, register_t retaddr,
                          register_t arg0, register_t arg1) {
  frame->ra = retaddr;
  frame->a0 = arg0;
  frame->a1 = arg1;
}

void exc_frame_set_retval(exc_frame_t *frame, register_t value,
                          register_t error) {
  frame->v0 = (register_t)value;
  frame->v1 = (register_t)error;
  frame->pc += 4;
}

int do_setcontext(thread_t *td, ucontext_t *uc) {
  mcontext_t *from = &uc->uc_mcontext;
  exc_frame_t *to = td->td_uframe;

  /* registers AT-PC */
  memcpy(&to->at, &from->__gregs[_REG_AT],
         sizeof(__greg_t) * (_REG_EPC - _REG_AT + 1));

  /* 32 FP registers + FP CSR */
  memcpy(&to->f0, &from->__fpregs.__fp_r,
         sizeof(from->__fpregs.__fp_r) + sizeof(from->__fpregs.__fp_csr));

  return EJUSTRETURN;
}
