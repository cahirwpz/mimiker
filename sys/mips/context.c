#include <sys/libkern.h>
#include <sys/context.h>
#include <sys/exception.h>
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

void exc_frame_setup_call(exc_frame_t *frame, void *ra, long arg0, long arg1) {
  frame->ra = (register_t)ra;
  frame->a0 = (register_t)arg0;
  frame->a1 = (register_t)arg1;
}

void exc_frame_set_retval(exc_frame_t *frame, long value) {
  frame->v0 = (register_t)value;
  frame->pc += 4;
}
