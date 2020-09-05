#include <sys/libkern.h>
#include <sys/mimiker.h>
#include <sys/thread.h>
#include <aarch64/context.h>
#include <aarch64/armreg.h>

void user_ctx_init(user_ctx_t *ctx, void *pc, void *sp) {
  panic("Not implemented!");
}

void user_ctx_copy(user_ctx_t *to, user_ctx_t *from) {
  panic("Not implemented!");
}

void user_ctx_setup_call(user_ctx_t *ctx, register_t retaddr, register_t arg) {
  panic("Not implemented!");
}

void user_ctx_set_retval(user_ctx_t *ctx, register_t value, register_t error) {
  panic("Not implemented!");
}

void user_exc_leave(void) {
  panic("Not implemented!");
}

void ctx_init(ctx_t *ctx, void *pc, void *sp) {
  bzero(ctx, sizeof(ctx_t));

  _REG(ctx, PC) = (register_t)pc;
  _REG(ctx, SP) = (register_t)sp;
  
  _REG(ctx, SPSR) = PSR_F | PSR_M_EL1h;
}

void ctx_set_retval(ctx_t *ctx, long value) {
  _REG(ctx, X0) = value;
}

long ctx_switch(thread_t *from, thread_t *to) {
  panic("Not implemented!");
}
