#include <sys/context.h>
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

register_t ctx_get_pc(ctx_t *ctx) {
  return _REG(ctx, PC);
}

void ctx_set_pc(ctx_t *ctx, uintptr_t addr) {
  _REG(ctx, PC) = addr;
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

__no_profile bool user_mode_p(ctx_t *ctx) {
  return (_REG(ctx, SPSR) & PSR_M_MASK) == PSR_M_EL0t;
}

int do_setcontext(thread_t *td, ucontext_t *uc) {
  mcontext_t *from = &uc->uc_mcontext;
  mcontext_t *to = td->td_uctx;

  if (uc->uc_flags & _UC_CPU) {
    register_t spsr = _REG(from, SPSR);
    /* Validate CPU context. */
    if (spsr & ~PSR_NZCV)
      return EINVAL;

    /* Allow only NZCV bits modification. */
    spsr |= _REG(to, SPSR) & ~PSR_NZCV;
    _REG(from, SPSR) = spsr;

    memcpy(&to->__gregs, &from->__gregs, sizeof(__gregset_t));
  }

  /* 32 FP registers + FPCR + FPSR */
  if (uc->uc_flags & _UC_FPU)
    memcpy(&to->__fregs, &from->__fregs, sizeof(__fregset_t));

  /*
   * We call do_setcontext only from sys_setcontext.
   *
   * User-space non-local jumps assume that lr contains return address for
   * setcontext syscall, but exception handler copies return address from elr
   * register.
   */
  _REG(to, ELR) = _REG(to, LR);

  return EJUSTRETURN;
}
