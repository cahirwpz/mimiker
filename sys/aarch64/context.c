#include <sys/mimiker.h>
#include <sys/thread.h>
#include <aarch64/context.h>
#include <sys/libkern.h>
#include <aarch64/armreg.h>

void ctx_init(ctx_t *ctx, void *pc, void *sp) {
  bzero(ctx, sizeof(ctx_t));

  ctx->pc = (register_t)pc;
  ctx->sp = (register_t)sp;

  ctx->spsr = PSR_F | PSR_M_EL1h;
}

void ctx_set_retval(ctx_t *ctx, long value) {
  ctx->x[0] = value;
}

long ctx_switch(thread_t *from, thread_t *to) {
  panic("Not implemented!");
}
