#include <sys/libkern.h>
#include <sys/context.h>
#include <mips/mips.h>
#include <mips/context.h>
#include <mips/pmap.h>

void ctx_init(ctx_t *ctx, void *pc, void *sp, unsigned flags) {
  bool usermode = flags & EF_USER;

  bzero(ctx, usermode ? sizeof(user_ctx_t) : sizeof(ctx_t));

  ctx->gp = usermode ? 0 : (register_t)_gp;
  ctx->pc = (register_t)pc;
  ctx->sp = (register_t)sp;

  /* For user-mode context we must make sure that:
   * - user mode is active,
   * - interrupts are enabled.
   * The rest will be covered by usr_exc_leave. */
  ctx->sr = mips32_get_c0(C0_STATUS) | SR_IE | (usermode ? SR_KSU_USER : 0);
}

void user_ctx_copy(user_ctx_t *to, user_ctx_t *from) {
  memcpy(to, from, sizeof(user_ctx_t));
}

void user_ctx_set_retval(user_ctx_t *ctx, register_t value, register_t error) {
  ctx->v0 = (register_t)value;
  ctx->v1 = (register_t)error;
  ctx->pc += 4;
}

void ctx_setup_call(ctx_t *ctx, register_t retaddr, register_t arg) {
  ctx->ra = retaddr;
  ctx->a0 = arg;
}

void ctx_set_retval(ctx_t *ctx, long value) {
  ctx->v0 = (register_t)value;
}
