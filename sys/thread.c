#include <stdarg.h>
#include <stdc.h>
#include <malloc.h>
#include <thread.h>
#include <context.h>
#include <interrupt.h>
#include <pcpu.h>

static MALLOC_DEFINE(td_pool, "kernel threads pool");

noreturn void thread_init(void (*fn)(), int n, ...) {
  thread_t *td;

  kmalloc_init(td_pool);
  kmalloc_add_arena(td_pool, pm_alloc(1)->vaddr, PAGESIZE);

  td = thread_create("main", fn, TD_KERNEL);

  /* Pass arguments to called function. */
  exc_frame_t *kframe = td->td_kframe;
  va_list ap;

  assert(n <= 4);
  va_start(ap, n);
  for (int i = 0; i < n; i++)
    (&kframe->a0)[i] = va_arg(ap, reg_t);
  va_end(ap);

  kprintf("[thread] Activating '%s' {%p} thread!\n", td->td_name, td);
  td->td_state = TDS_RUNNING;
  ctx_boot(td);
}

thread_t *thread_create(const char *name, void (*fn)(), td_type_t td_type) {
  thread_t *td = kmalloc(td_pool, sizeof(thread_t), M_ZERO);
<<<<<<< af4ad4630942bc57e0a2e8c6b5cba5b1bab2fbd3
=======
  
  if(td_type == TD_USER)
      td->user_map = vm_map_new(USER_VM_MAP, 0); //TODO what's witch asid?
>>>>>>> Add user_map to thread.c
  td->td_type = td_type;
  td->td_name = name;
  td->td_kstack_obj = pm_alloc(1);
  td->td_kstack.stk_base = (void *)PG_VADDR_START(td->td_kstack_obj);
  td->td_kstack.stk_size = PAGESIZE;

  ctx_init(td, fn);

  td->td_state = TDS_READY;

  return td;
}

void thread_delete(thread_t *td) {
  assert(td != NULL);
  assert(td != thread_self());

  pm_free(td->td_kstack_obj);
  kfree(td_pool, td);
}

thread_t *thread_self() {
  return PCPU_GET(curthread);
}

void thread_switch_to(thread_t *newtd) {
  thread_t *td = thread_self();

  if (newtd == NULL || newtd == td)
    return;

  /* Thread must not switch while in critical section! */
  assert(td->td_csnest == 0);

  log("Switching from '%s' {%p} to '%s' {%p}.", td->td_name, td, newtd->td_name,
      newtd);

  td->td_state = TDS_READY;
  newtd->td_state = TDS_RUNNING;
  ctx_switch(td, newtd);
}
