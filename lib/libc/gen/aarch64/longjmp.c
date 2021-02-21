#include <setjmp.h>
#include <sys/types.h>
#include <ucontext.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

#define _REG(ctx, n) ((ctx)->uc_mcontext.__gregs[_REG_##n])

void longjmp(jmp_buf env, int val) {
  ucontext_t *sc_uc = (void *)env;
  ucontext_t uc;
  memset(&uc, 0, sizeof(ucontext_t));

  /* Ensure non-zero SP */
  if (_REG(sc_uc, SP) == 0)
    goto err;

  /* Ensure non-zero return value */
  if (val == 0)
    val = 1;

  uc.uc_flags =
    _UC_CPU | ((sc_uc->uc_flags & _UC_STACK) ? _UC_SETSTACK : _UC_CLRSTACK);

  sigprocmask(SIG_SETMASK, &sc_uc->uc_sigmask, NULL);

  /* Clear uc_link */
  uc.uc_link = 0;

  /* Save return value in context */
  _REG(&uc, X0) = val;

  /* Copy saved registers */
  _REG(&uc, X19) = _REG(sc_uc, X19);
  _REG(&uc, X20) = _REG(sc_uc, X20);
  _REG(&uc, X21) = _REG(sc_uc, X21);
  _REG(&uc, X22) = _REG(sc_uc, X22);
  _REG(&uc, X23) = _REG(sc_uc, X23);
  _REG(&uc, X24) = _REG(sc_uc, X24);
  _REG(&uc, X25) = _REG(sc_uc, X25);
  _REG(&uc, X26) = _REG(sc_uc, X26);
  _REG(&uc, X27) = _REG(sc_uc, X27);
  _REG(&uc, X28) = _REG(sc_uc, X28);
  _REG(&uc, X29) = _REG(sc_uc, X29);

  _REG(&uc, SP) = _REG(sc_uc, SP);
  _REG(&uc, LR) = _REG(sc_uc, LR);
  _REG(&uc, PC) = _REG(sc_uc, PC);
  _REG(&uc, SPSR) = _REG(sc_uc, SPSR);
  _REG(&uc, TPIDR) = _REG(sc_uc, TPIDR);

  if (sc_uc->uc_flags & _UC_FPU) {
    memcpy(&uc.uc_mcontext.__fregs, &sc_uc->uc_mcontext.__fregs,
           sizeof(__fregset_t));
    uc.uc_flags |= _UC_FPU;
  }

  setcontext(&uc);
err:
  longjmperror();
  abort();
  /* NOTREACHED */
}
