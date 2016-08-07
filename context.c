#include <libkern.h>
#include <context.h>

void ctx_init(ctx_t *ctx, void (*target)(), void *sp, bool prepare_stack) {
  register void *gp asm("$gp");

  bzero(ctx, sizeof(ctx_t));

  ctx->reg[REG_RA] = (intptr_t)target;
  ctx->reg[REG_GP] = (intptr_t)gp;
  ctx->reg[REG_SP] = (intptr_t)sp;
}

void* ctx_stack_push(ctx_t *ctx, size_t size) {
  ctx->reg[REG_SP] -= size;
  void *sp = (void *)ctx->reg[REG_SP];
  memset(sp, 0, size);
  return sp;
}

void ctx_switch(ctx_t *from, ctx_t *to) {
  if (!ctx_save(from))
    ctx_load(to);
}
