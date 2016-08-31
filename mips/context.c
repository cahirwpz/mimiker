#include <stdc.h>
#include <context.h>

extern void kernel_exit();

void ctx_init(ctx_t *ctx, void (*target)(), void *sp) {
  register void *gp asm("$gp");

  bzero(ctx, sizeof(ctx_t));

  ctx->reg[REG_PC] = (reg_t)target;
  ctx->reg[REG_GP] = (reg_t)gp;
  ctx->reg[REG_SP] = (reg_t)sp;
  ctx->reg[REG_RA] = (reg_t)kernel_exit;
}

void ctx_switch(ctx_t *from, ctx_t *to) {
  if (!ctx_save(from))
    ctx_load(to);
}
