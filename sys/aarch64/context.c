#include <sys/mimiker.h>
#include <sys/thread.h>
#include <aarch64/context.h>

void ctx_init(ctx_t *ctx, void *pc, void *sp) {
  panic("Not implemented!");
}

void ctx_set_retval(ctx_t *ctx, long value) {
  panic("Not implemented!");
}

long ctx_switch(thread_t *from, thread_t *to) {
  panic("Not implemented!");
}
