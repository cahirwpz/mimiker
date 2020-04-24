#include <sys/libkern.h>
#include <mips/context.h>
#include <mips/m32c0.h>
#include <mips/pmap.h>

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
