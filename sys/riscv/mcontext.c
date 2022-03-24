#include <sys/context.h>
#include <sys/klog.h>

bool user_mode_p(ctx_t *ctx) {
  panic("Not implemented!");
}

void ctx_init(ctx_t *ctx, void *pc, void *sp) {
  panic("Not implemented!");
}

void ctx_setup_call(ctx_t *ctx, register_t retaddr, register_t arg) {
  panic("Not implemented!");
}

void ctx_set_retval(ctx_t *ctx, long value) {
  panic("Not implemented!");
}

register_t ctx_get_pc(ctx_t *ctx) {
  panic("Not implemented!");
}

void mcontext_copy(mcontext_t *to, mcontext_t *from) {
  panic("Not implemented!");
}

void mcontext_init(mcontext_t *ctx, void *pc, void *sp) {
  panic("Not implemented!");
}

void mcontext_set_retval(mcontext_t *ctx, register_t value, register_t error) {
  panic("Not implemented!");
}

void mcontext_restart_syscall(mcontext_t *ctx) {
  panic("Not implemented!");
}

long ctx_switch(thread_t *from, thread_t *to) {
  panic("Not implemented!");
}

int do_setcontext(thread_t *td, ucontext_t *uc) {
  panic("Not implemented!");
}
