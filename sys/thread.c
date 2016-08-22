#include <stdarg.h>
#include <stdc.h>
#include <malloc.h>
#include <thread.h>
#include <context.h>
#include <interrupts.h>

static thread_t *td_running = NULL;

thread_t *thread_self() {
  return td_running;
}

static MALLOC_DEFINE(td_pool, "kernel threads pool");

extern void irq_return();
extern void kernel_exit();

noreturn void thread_init(void (*fn)(), int n, ...) {
  thread_t *td;

  kmalloc_init(td_pool);
  kmalloc_add_arena(td_pool, pm_alloc(1)->vaddr, PAGESIZE);

  td = thread_create("main", fn);

  /* Pass arguments to called function. */
  ctx_t *irq_ctx = (ctx_t *)td->td_context.reg[REG_SP];
  va_list ap;

  assert(n <= 4);
  va_start(ap, n);
  for (int i = 0; i < n; i++)
    irq_ctx->reg[REG_A0 + i] = va_arg(ap, intptr_t);
  va_end(ap);

  kprintf("[thread] Activating '%s' {%p} thread!\n", td->td_name, td);
  td->td_state = TDS_RUNNING;
  td_running = td;
  ctx_load(&td->td_context);
}

thread_t *thread_create(const char *name, void (*fn)()) {
  thread_t *td = kmalloc(td_pool, sizeof(thread_t), M_ZERO);
  
  td->td_name = name;
  td->td_stack = pm_alloc(1);
  td->td_state = TDS_READY;

  ctx_init(&td->td_context, irq_return, (void *)PG_VADDR_END(td->td_stack));

  /* This context will be used by 'irq_return'. */
  ctx_t *irq_ctx = ctx_stack_push(&td->td_context, sizeof(ctx_t));

  /* In supervisor mode CPU may use ERET instruction even if Status.EXL = 0. */
  irq_ctx->reg[REG_EPC] = (intptr_t)fn;
  irq_ctx->reg[REG_RA] = (intptr_t)kernel_exit;

  return td;
}

void thread_delete(thread_t *td) {
  assert(td != td_running);

  pm_free(td->td_stack);
  kfree(td_pool, td);
}

void thread_switch_to(thread_t *td_ready) {
  if (!td_ready)
    return;

  /* TODO: This must be done with interrupts disabled! */
  log("Switching from '%s' {%p} to '%s' {%p}.",
      td_running->td_name, td_running, td_ready->td_name, td_ready);
  assert(td_running != td_ready);

  swap(td_running, td_ready);
  td_running->td_state = TDS_RUNNING;
  td_ready->td_state = TDS_READY;
  ctx_switch(&td_ready->td_context, &td_running->td_context);
}
