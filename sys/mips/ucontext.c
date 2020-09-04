#include <sys/libkern.h>
#include <sys/thread.h>
#include <sys/errno.h>
#include <sys/ucontext.h>
#include <mips/context.h>

int do_setcontext(thread_t *td, ucontext_t *uc) {
  mcontext_t *from = &uc->uc_mcontext;
  user_ctx_t *to = td->td_uctx;

  /* registers AT-PC */
  memcpy(&to->at, &from->__gregs[_REG_AT],
         sizeof(__greg_t) * (_REG_EPC - _REG_AT + 1));

  /* 32 FP registers + FP CSR */
  memcpy(&to->f0, &from->__fpregs.__fp_r,
         sizeof(from->__fpregs.__fp_r) + sizeof(from->__fpregs.__fp_csr));

  return EJUSTRETURN;
}
