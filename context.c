#include <libkern.h>
#include <context.h>

void ctx_init(ctx_t *ctx, void (*target)(), void *sp) {
  register void *gp asm("$gp");

  bzero(ctx, sizeof(ctx_t));
  ctx->ra = (intptr_t)target;
  ctx->sp = (intptr_t)sp;
  ctx->gp = (intptr_t)gp;
}

void ctx_switch(ctx_t *from, ctx_t *to) {
  if (!ctx_save(from))
    ctx_load(to);
}

void ctx_switch_interrupt(ctx_t *from, ctx_t *to) {
  if (!ctx_save(from))
    ctx_load_interrupt(to);
}
