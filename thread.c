#include <libkern.h>
#include <malloc.h>
#include <thread.h>
#include <context.h>
#include <interrupts.h>

thread_t *td_running = NULL;

static MALLOC_DEFINE(td_pool, "kernel threads pool");

_Noreturn void thread_init(void (*fn)(), int argc, ...) {
  kmalloc_init(td_pool);
  kmalloc_add_arena(td_pool, pm_alloc(1)->vaddr, PAGESIZE);

  td_running = kmalloc(td_pool, sizeof(thread_t), M_ZERO);
  td_running->td_stack = pm_alloc(1);
  td_running->td_state = TDS_RUNNING;

  extern void kernel_exit();
  ctx_init(&td_running->td_context, kernel_exit,
           (void *)PG_VADDR_END(td_running->td_stack));

  log("[thread] Activating first thread at %p.", td_running);
  /* TODO: How to pass arguments to called function? */
  ctx_call(&td_running->td_context, fn);
}

thread_t *thread_create(void (*fn)()) {
  thread_t *td = kmalloc(td_pool, sizeof(thread_t), M_ZERO);
  td->td_stack = pm_alloc(1);
  td->td_state = TDS_NEW;
  ctx_init(&td->td_context, fn, (void *)PG_VADDR_END(td->td_stack));
  return td;
}

void thread_delete(thread_t *td) {
  assert(td != td_running);

  pm_free(td->td_stack);
  kfree(td_pool, td);
}

thread_t* thread_switch_to(thread_t *td_ready) {
  /* TODO: This must be done with interrupts disabled! */
  log("Switching threads from %p to %p.", td_running, td_ready);
  assert(td_running != td_ready);

  bool first_time = (td_ready->td_state == TDS_NEW);
  swap(td_running, td_ready);
  td_running->td_state = TDS_RUNNING;
  td_ready->td_state = TDS_READY;
  if (first_time)
    ctx_switch_interrupt(&td_ready->td_context, &td_running->td_context);
  else
    ctx_switch(&td_ready->td_context, &td_running->td_context);

  return td_ready;
}

#ifdef _KERNELSPACE
static thread_t *td0, *td1, *td2;

static void demo_thread_1() {
  int times = 3;

  kprintf("Thread #1 has started.\n");

  do {
    thread_switch_to(td2);
    kprintf("Running in thread #1.\n");
  } while (--times);

  thread_switch_to(td0);
}

static void demo_thread_2() {
  int times = 3;

  kprintf("Thread #2 has started.\n");

  do {
    thread_switch_to(td1);
    kprintf("Running in thread #2.\n");
  } while (--times);

  panic("This line need not be reached!");
}

int main() {
  kprintf("Main context has started.\n");

  td0 = td_running;
  td1 = thread_create(demo_thread_1);
  td2 = thread_create(demo_thread_2);

  thread_switch_to(td1);

  kprintf("Main thread continuing.\n");

  thread_delete(td2);
  thread_delete(td1);

  return 0;
}

#endif
