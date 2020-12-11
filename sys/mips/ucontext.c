#include <sys/libkern.h>
#include <sys/thread.h>
#include <sys/errno.h>
#include <sys/ucontext.h>

int do_setcontext(thread_t *td, ucontext_t *uc) {
  mcontext_t *from = &uc->uc_mcontext;
  mcontext_t *to = td->td_uctx;

  /* registers AT-PC */
  memcpy(&_REG(to, AT), &_REG(from, AT),
         sizeof(__greg_t) * (_REG_EPC - _REG_AT + 1));

  /* 32 FP registers + FP CSR */
  memcpy(&to->__fpregs.__fp_r, &from->__fpregs.__fp_r,
         sizeof(from->__fpregs.__fp_r) + sizeof(from->__fpregs.__fp_csr));

  return EJUSTRETURN;
}
