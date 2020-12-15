#include <sys/libkern.h>
#include <sys/mimiker.h>
#include <sys/thread.h>
#include <sys/context.h>
#include <aarch64/armreg.h>
#include <aarch64/mcontext.h>
#include <sys/ucontext.h>
#include <sys/errno.h>

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

bool user_mode_p(ctx_t *ctx) {
  return (_REG(ctx, SPSR) & PSR_M_MASK) == PSR_M_EL0t;
}

int do_setcontext(thread_t *td, ucontext_t *uc) {
  mcontext_t *from = &uc->uc_mcontext;
  mcontext_t *to = td->td_uctx;

  if (uc->uc_flags & _UC_CPU)
    memcpy(&to->__gregs, &from->__gregs, sizeof(__gregset_t));

  /* 32 FP registers + FPCR + FPSR */
  if (uc->uc_flags & _UC_FPU)
    memcpy(&to->__fregs, &from->__fregs, sizeof(__fregset_t));

  return EJUSTRETURN;
}
