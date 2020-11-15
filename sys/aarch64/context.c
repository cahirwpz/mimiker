#include <sys/libkern.h>
#include <aarch64/context.h>
#include <aarch64/armreg.h>

void ctx_init(ctx_t *ctx, void *pc, void *sp) {
  bzero(ctx, sizeof(ctx_t));

  _REG(ctx, PC) = (register_t)pc;
  _REG(ctx, SP) = (register_t)sp;

  _REG(ctx, SPSR) = PSR_F | PSR_M_EL1h;
}

void ctx_setup_call(ctx_t *ctx, register_t retaddr, register_t arg) {
  _REG(ctx, LR) = retaddr;
  _REG(ctx, X0) = arg;
}

void ctx_set_retval(ctx_t *ctx, long value) {
  _REG(ctx, X0) = value;
}

void user_ctx_copy(user_ctx_t *to, user_ctx_t *from) {
  memcpy(to, from, sizeof(user_ctx_t));
}

void user_ctx_init(user_ctx_t *ctx, void *pc, void *sp) {
  bzero(ctx, sizeof(user_ctx_t));

  _REG(ctx, PC) = (register_t)pc;
  _REG(ctx, SP) = (register_t)sp;
}

void user_ctx_set_retval(user_ctx_t *ctx, register_t value, register_t error) {
  _REG(ctx, X0) = value;
  _REG(ctx, X1) = error;
  _REG(ctx, PC) += 4;
}

bool user_mode_p(ctx_t *ctx) {
  /* XXX Not implemented! */
  return false;
}
