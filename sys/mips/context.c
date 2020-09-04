#include <sys/libkern.h>
#include <sys/context.h>
#include <mips/mips.h>
#include <mips/context.h>
#include <mips/pmap.h>

void ctx_init(ctx_t *ctx, void *pc, void *sp) {
  bzero(ctx, sizeof(ctx_t));

  ctx->__gregs[_REG_GP] = (register_t)_gp;
  ctx->__gregs[_REG_EPC] = (register_t)pc;
  ctx->__gregs[_REG_SP] = (register_t)sp;

  /* Take SR from caller's context and enable interrupts. */
  ctx->__gregs[_REG_SR] = mips32_get_c0(C0_STATUS) | SR_IE;
}

void ctx_setup_call(ctx_t *ctx, register_t retaddr, register_t arg) {
  ctx->__gregs[_REG_RA] = retaddr;
  ctx->__gregs[_REG_A0] = arg;
}

void ctx_set_retval(ctx_t *ctx, long value) {
  ctx->__gregs[_REG_V0] = (register_t)value;
}

void user_ctx_copy(user_ctx_t *to, user_ctx_t *from) {
  memcpy(to, from, sizeof(user_ctx_t));
}

void user_ctx_init(user_ctx_t *ctx, void *pc, void *sp) {
  bzero(ctx, sizeof(user_ctx_t));

  ctx->__gregs[_REG_GP] = (register_t)0;
  ctx->__gregs[_REG_EPC] = (register_t)pc;
  ctx->__gregs[_REG_SP] = (register_t)sp;

  /* For user-mode context we must make sure that:
   * - user mode is active,
   * - interrupts are enabled.
   * The rest will be covered by usr_exc_leave. */
  ctx->__gregs[_REG_SR] = mips32_get_c0(C0_STATUS) | SR_IE | SR_KSU_USER;
}

void user_ctx_set_retval(user_ctx_t *ctx, register_t value, register_t error) {
  ctx->__gregs[_REG_V0] = (register_t)value;
  ctx->__gregs[_REG_V1] = (register_t)error;
  ctx->__gregs[_REG_EPC] += 4;
}
