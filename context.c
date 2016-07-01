#include <libkern.h>
#include <context.h>
#include <pm.h>

void ctx_init(ctx_t *ctx, void (*target)(), stack_t *stk) {
  register void *gp asm("$gp");

  bzero(ctx, sizeof(ctx_t));
  ctx->ra = (intptr_t)target;
  ctx->sp = (intptr_t)(stk->stk_base + stk->stk_size);
  ctx->gp = (intptr_t)gp;
}

void ctx_switch(ctx_t *from, ctx_t *to) {
  if (!ctx_save(from))
    ctx_load(to);
}

#ifdef _KERNELSPACE
static ctx_t ctx0, ctx1, ctx2;

static void demo_context_1() {
  int times = 3;

  kprintf("Context #1 has started.\n");

  do {
    ctx_switch(&ctx1, &ctx2);
    kprintf("Running in context #1.\n");
  } while (--times);

  ctx_switch(&ctx1, &ctx0);
}

static void demo_context_2() {
  int times = 3;

  kprintf("Context #2 has started.\n");

  do {
    ctx_switch(&ctx2, &ctx1);
    kprintf("Running in context #2.\n");
  } while (--times);

  // NOT REACHED
}

int main() {
  kprintf("Main context has started.\n");

  vm_page_t *pg1 = pm_alloc(1);
  vm_page_t *pg2 = pm_alloc(1);

  stack_t stk1 = { (void *)pg1->virt_addr, PAGESIZE };
  stack_t stk2 = { (void *)pg2->virt_addr, PAGESIZE };

  // Prepare alternative contexts
  ctx_init(&ctx1, demo_context_1, &stk1);
  ctx_init(&ctx2, demo_context_2, &stk2);

  // Switch to context 1
  ctx_switch(&ctx0, &ctx1);

  kprintf("Main context continuing.\n");

  pm_free(pg1);
  pm_free(pg2);

  return 0;
}

#endif

