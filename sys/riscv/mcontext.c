#include <sys/context.h>
#include <sys/errno.h>
#include <sys/klog.h>
#include <sys/libkern.h>
#include <sys/mimiker.h>
#include <sys/pcpu.h>
#include <sys/thread.h>
#include <sys/ucontext.h>
#include <riscv/mcontext.h>
#include <riscv/riscvreg.h>

#ifdef __riscv_c
#error "Mimiker assumes four-byte instructions!"
#endif

#define __gp()                                                                 \
  ({                                                                           \
    register_t __rv;                                                           \
    __asm __volatile("mv %0, gp" : "=r"(__rv));                                \
    __rv;                                                                      \
  })

void ctx_init(ctx_t *ctx, void *pc, void *sp) {
  bzero(ctx, sizeof(ctx_t));

  _REG(ctx, GP) = __gp();
  _REG(ctx, TP) = (register_t)_pcpu_data;
  _REG(ctx, PC) = (register_t)pc;
  _REG(ctx, SP) = (register_t)sp;

  /*
   * Supervisor status register:
   *  - Make executable readable = FALSE
   *  - Permit supervisor user memory access = FALSE
   *  - Floating point extension state = OFF
   *  - Supervisor previous privilege mode = SUPERVISOR
   *  - Supervisor previous interrupt enabled = TRUE
   *  - Supervisor interrupt enabled = FALSE
   */
  _REG(ctx, SR) = SSTATUS_FS_OFF | SSTATUS_SPP_SUPV | SSTATUS_SPIE;
}

void ctx_setup_call(ctx_t *ctx, register_t retaddr, register_t arg) {
  _REG(ctx, RA) = retaddr;
  _REG(ctx, A0) = arg;
}

void ctx_set_retval(ctx_t *ctx, long value) {
  _REG(ctx, RV) = (register_t)value;
}

register_t ctx_get_pc(ctx_t *ctx) {
  return _REG(ctx, PC);
}

void mcontext_copy(mcontext_t *to, mcontext_t *from) {
  memcpy(to, from, sizeof(mcontext_t));
}

void mcontext_init(mcontext_t *ctx, void *pc, void *sp) {
  bzero(ctx, sizeof(mcontext_t));

  /* NOTE: global pointer will be set by the csu library. */
  _REG(ctx, PC) = (register_t)pc;
  _REG(ctx, SP) = (register_t)sp;

  /*
   * Supervisor status register:
   *  - Make executable readable = FALSE
   *  - Permit supervisor user memory access = FALSE
   *  - Floating point extension state = OFF
   *  - Supervisor previous privilege mode = USER
   *  - Supervisor previous interrupt enabled = TRUE
   *  - Supervisor interrupt enabled = FALSE
   */
  _REG(ctx, SR) = SSTATUS_FS_OFF | SSTATUS_SPP_USER | SSTATUS_SPIE;
}

void mcontext_set_retval(mcontext_t *ctx, register_t value, register_t error) {
  _REG(ctx, A0) = value;
  _REG(ctx, A1) = error;
  _REG(ctx, PC) += 4;
}

void mcontext_restart_syscall(mcontext_t *ctx) {
  /* Nothing needs to be done. */
}

__no_profile bool user_mode_p(ctx_t *ctx) {
  return (_REG(ctx, SR) & SSTATUS_SPP_MASK) == SSTATUS_SPP_USER;
}

int do_setcontext(thread_t *td, ucontext_t *uc) {
  mcontext_t *from = &uc->uc_mcontext;
  mcontext_t *to = td->td_uctx;

  /* registers RA-PC */
  if (uc->uc_flags & _UC_CPU) {
    size_t gregsz = sizeof(__greg_t) * (_REG_PC - _REG_RA + 1);
    memcpy(&_REG(to, RA), &_REG(from, RA), gregsz);
  }

#if FPU
  /* FPE state */
  if (uc->uc_flags & _UC_FPU)
    memcpy(to->__fregs, from->__fregs, sizeof(__fregset_t));
#endif

  return EJUSTRETURN;
}
