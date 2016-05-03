#include <context.h>
#include <libkern.h>

static ctx_t ctx0, ctx1, ctx2;
static intptr_t stack1[200];
static intptr_t stack2[200];

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

void demo_ctx() {
  kprintf("Main context has started.\n");

  // Prepare alternative contexts
  register void *gp asm("$gp");
  ctx_init(&ctx1, demo_context_1, stack1 + 199, gp);
  ctx_init(&ctx2, demo_context_2, stack2 + 199, gp);

  // Switch to context 1
  ctx_switch(&ctx0, &ctx1);

  kprintf("Main context continuing.\n");
}
