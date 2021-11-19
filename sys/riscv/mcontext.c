#include <sys/context.h>
#include <sys/klog.h>
#include <sys/libkern.h>
#include <sys/mimiker.h>
#include <riscv/mcontext.h>

#define __gp()                                                                 \
  ({                                                                           \
    register_t __rv;                                                           \
    __asm __volatile("mv %0, gp" : "=r"(__rv));                                \
    __rv;                                                                      \
  })

void ctx_init(ctx_t *ctx, void *pc, void *sp) {
  bzero(ctx, sizeof(ctx_t));

  _REG(ctx, GP) = __gp();
  _REG(ctx, PC) = (register_t)pc;
  _REG(ctx, SP) = (register_t)sp;
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
  _REG(ctx, GP) = (register_t)0;
  _REG(ctx, PC) = (register_t)pc;
  _REG(ctx, SP) = (register_t)sp;
}

void mcontext_set_retval(mcontext_t *ctx, register_t value, register_t error) {
  _REG(ctx, X10) = value;
  _REG(ctx, X11) = error;
  /* XXX: we assume the C extension is not supported. */
  _REG(ctx, PC) += 4;
}

void mcontext_restart_syscall(mcontext_t *ctx) {
  /* Nothing needs to be done. */
}

__no_profile bool user_mode_p(ctx_t *ctx) {
  panic("Not implemented!");
}

long ctx_switch(thread_t *from, thread_t *to) {
  panic("Not implemented!");
}

int do_setcontext(thread_t *td, ucontext_t *uc) {
  panic("Not implemented!");
}
