#include <sys/context.h>
#include <sys/libkern.h>
#include <sys/mimiker.h>
#include <sys/thread.h>
#include <sys/context.h>
#include <aarch64/armreg.h>
#include <aarch64/mcontext.h>

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

void mcontext_copy(mcontext_t *to, mcontext_t *from) {
  memcpy(to, from, sizeof(mcontext_t));
}

void mcontext_init(mcontext_t *ctx, void *pc, void *sp) {
  bzero(ctx, sizeof(mcontext_t));

  _REG(ctx, PC) = (register_t)pc;
  _REG(ctx, SP) = (register_t)sp;
}

void mcontext_set_retval(mcontext_t *ctx, register_t value, register_t error) {
  _REG(ctx, X0) = value;
  _REG(ctx, X1) = error;
}

void mcontext_restart_syscall(mcontext_t *ctx) {
  _REG(ctx, PC) -= 4; /* TODO subtract 2 if in thumb mode */
}

bool user_mode_p(ctx_t *ctx) {
  return (_REG(ctx, SPSR) & PSR_M_MASK) == PSR_M_EL0t;
}

int do_setcontext(thread_t *td, ucontext_t *uc) {
  panic("Not implemented!");
}
