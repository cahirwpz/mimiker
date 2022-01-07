#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <sys/types.h>

#define _REG(ctx, n) ((ctx)->uc_mcontext.__gregs[_REG_##n])
#define _FPREG(ctx, n) ((ctx)->uc_mcontext.__fregs[(n)])
#define _FPCSR(ctx) _FPREG(ctx, _REG_FPCSR)

void longjmp(jmp_buf env, int val) {
  ucontext_t *sc_uc = (ucontext_t *)env;
  ucontext_t uc;

  bzero(&uc, sizeof(ucontext_t));

  /* Ensure non-zero SP. */
  if (!_REG(sc_uc, SP))
    goto err;

  /* Ensure non-zero return vaule. */
  val = val ? val : 1;

  /*
   * Set _UC_{SET,CLR}STACK according to SS_ONSTACK.
   *
   * Restore the signal mask with sigprocmask() instead of _UC_SIGMASK,
   * since libpthread may want to interpose on signal handling.
   */
  uc.uc_flags =
    _UC_CPU | ((sc_uc->uc_flags & _UC_STACK) ? _UC_SETSTACK : _UC_CLRSTACK);

  /* Restore sigmask. */
  sigprocmask(SIG_SETMASK, &sc_uc->uc_sigmask, NULL);

  /* Save return value in context. */
  _REG(&uc, RV) = val;

  /* Copy saved registers. */
  _REG(&uc, RA) = _REG(sc_uc, RA);
  _REG(&uc, SP) = _REG(sc_uc, SP);
  _REG(&uc, GP) = _REG(sc_uc, GP);
  _REG(&uc, TP) = _REG(sc_uc, TP);
  _REG(&uc, S0) = _REG(sc_uc, S0);
  _REG(&uc, S1) = _REG(sc_uc, S1);
  _REG(&uc, S2) = _REG(sc_uc, S2);
  _REG(&uc, S3) = _REG(sc_uc, S3);
  _REG(&uc, S4) = _REG(sc_uc, S4);
  _REG(&uc, S5) = _REG(sc_uc, S5);
  _REG(&uc, S6) = _REG(sc_uc, S6);
  _REG(&uc, S7) = _REG(sc_uc, S7);
  _REG(&uc, S8) = _REG(sc_uc, S8);
  _REG(&uc, S9) = _REG(sc_uc, S9);
  _REG(&uc, S10) = _REG(sc_uc, S10);
  _REG(&uc, S11) = _REG(sc_uc, S11);
  _REG(&uc, PC) = _REG(sc_uc, PC);

#ifndef __riscv_float_abi_soft
  /* Copy FPE state. */
  if (sc_uc->uc_flags & _UC_FPU) {
    /* FP callee saved registers are: f8-9, f18-27. */
    memcpy(&_FPREG(&uc, 8), &_FPREG(sc_uc, 8), (10 - 8) * sizeof(__fpreg_t));
    memcpy(&_FPREG(&uc, 18), &_FPREG(sc_uc, 18), (28 - 18) * sizeof(__fpreg_t));
    _FPCSR(&uc) = _FPCSR(sc_uc);
    uc.uc_flags |= _UC_FPU;
  }
#endif

  setcontext(&uc);
err:
  longjmperror();
  abort();
  /* NOTREACHED */
}
