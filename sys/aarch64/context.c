#include <sys/mimiker.h>
#include <sys/thread.h>
#include <aarch64/context.h>

void user_ctx_init(user_ctx_t *ctx, void *pc, void *sp) {
  panic("Not implemented!");
}

void user_ctx_copy(user_ctx_t *to, user_ctx_t *from) {
  panic("Not implemented!");
}

void user_ctx_setup_call(user_ctx_t *ctx, register_t retaddr, register_t arg) {
  panic("Not implemented!");
}

void user_ctx_set_retval(user_ctx_t *ctx, register_t value, register_t error) {
  panic("Not implemented!");
}

void user_exc_leave(void) {
  panic("Not implemented!");
}

void ctx_init(ctx_t *ctx, void *pc, void *sp) {
  panic("Not implemented!");
}

void ctx_set_retval(ctx_t *ctx, long value) {
  panic("Not implemented!");
}

long ctx_switch(thread_t *from, thread_t *to) {
  panic("Not implemented!");
}
