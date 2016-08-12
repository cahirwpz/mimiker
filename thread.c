#include <libkern.h>
#include <malloc.h>
#include <thread.h>
#include <context.h>
#include <interrupts.h>

static thread_t *td_running = NULL;

thread_t *thread_self() {
  return td_running;
}

static MALLOC_DEFINE(td_pool, "kernel threads pool");

noreturn void thread_init(void (*fn)(), int argc, ...) {
  kmalloc_init(td_pool);
  kmalloc_add_arena(td_pool, pm_alloc(1)->vaddr, PAGESIZE);

  td_running = kmalloc(td_pool, sizeof(thread_t), M_ZERO);
  td_running->td_name = "main";
  td_running->td_stack = pm_alloc(1);
  td_running->td_state = TDS_RUNNING;

  extern void kernel_exit();
  ctx_init(&td_running->td_context, kernel_exit,
           (void *)PG_VADDR_END(td_running->td_stack));

  kprintf("[thread] Activating '%s' {%p} thread!\n", 
          td_running->td_name, td_running);
  /* TODO: How to pass arguments to called function? */
  ctx_call(&td_running->td_context, fn);
}

thread_t *thread_create(const char *name, void (*fn)()) {
  thread_t *td = kmalloc(td_pool, sizeof(thread_t), M_ZERO);
  td->td_name = name;
  td->td_stack = pm_alloc(1);
  td->td_state = TDS_READY;
  ctx_init(&td->td_context, fn, (void *)PG_VADDR_END(td->td_stack));
  return td;
}

void thread_delete(thread_t *td) {
  assert(td != td_running);

  pm_free(td->td_stack);
  kfree(td_pool, td);
}

void thread_switch_to(thread_t *td_ready) {
  /* TODO: This must be done with interrupts disabled! */
  log("Switching from '%s' {%p} to '%s' {%p}.",
      td_running->td_name, td_running, td_ready->td_name, td_ready);
  assert(td_running != td_ready);

  swap(td_running, td_ready);
  td_running->td_state = TDS_RUNNING;
  td_ready->td_state = TDS_READY;
  ctx_switch(&td_ready->td_context, &td_running->td_context);
}

#ifdef _KERNELSPACE
static thread_t *td0, *td1, *td2;

static void demo_thread_1() {
  int times = 3;

  kprintf("Thread '%s' started.\n", thread_self()->td_name);

  do {
    thread_switch_to(td2);
    kprintf("Thread '%s' running.\n", thread_self()->td_name);
  } while (--times);

  thread_switch_to(td0);
}

static void demo_thread_2() {
  int times = 3;

  kprintf("Thread '%s' started.\n", thread_self()->td_name);

  do {
    thread_switch_to(td1);
    kprintf("Thread '%s' running.\n", thread_self()->td_name);
  } while (--times);

  panic("This line need not be reached!");
}

int main() {
  kprintf("Thread '%s' started.\n", thread_self()->td_name);

  td0 = thread_self();
  td1 = thread_create("first", demo_thread_1);
  td2 = thread_create("second", demo_thread_2);

  thread_switch_to(td1);

  kprintf("Thread '%s' running.\n", thread_self()->td_name);

  thread_delete(td2);
  thread_delete(td1);

  return 0;
}

#endif
