#include <stdc.h>
#include <clock.h>
#include <thread.h>
#include <sched.h>
#include <vm_map.h>
#include <vm_pager.h>

#if 0
static void demo_thread_1() {
  while (true) {
    realtime_t now = clock_get();
    kprintf("[%8zu] Running '%s' thread.\n", (size_t)now,
            thread_self()->td_name);
    while (clock_get() < now + 20)
      ;
  }
}

static void demo_thread_2() {
  kprintf("Running '%s' thread. Let's yield!\n", thread_self()->td_name);
  sched_yield();
  demo_thread_1();
}

void main() {
  thread_t *t1 = thread_create("t1", demo_thread_1);
  thread_t *t2 = thread_create("t2", demo_thread_1);
  thread_t *t3 = thread_create("t3", demo_thread_1);
  thread_t *t4 = thread_create("t4", demo_thread_2);
  thread_t *t5 = thread_create("t5", demo_thread_2);

  sched_add(t1);
  sched_add(t2);
  sched_add(t3);
  sched_add(t4);
  sched_add(t5);

  sched_run();
}
#endif

#define USER_START 0x1234000
#define USER_END 0x1235000

#define KERNEL_START 0xc1230000
#define KERNEL_END 0xc1231000

void kernel_thread() {
  volatile int *ptr = (int *)(KERNEL_START);
  while (1) {
    *ptr = *ptr + 1;
    kprintf("thread: %s, val: %d\n", thread_self()->td_name, *ptr);
    sched_yield();
  }
}

void user_thread() {
  volatile int *ptr = (int *)(USER_START);
  while (1) {
    *ptr = *ptr + 1;
    kprintf("thread: %s, val: %d\n", thread_self()->td_name, *ptr);
    sched_yield();
  }
}

void main() {
  thread_t *t1 = thread_create("kernel-mode-thread-1", kernel_thread);
  thread_t *t2 = thread_create("kernel-mode-thread-2", kernel_thread);
  thread_t *t3 = thread_create("user-mode-thread-1", user_thread);
  thread_t *t4 = thread_create("user-mode-thread-2", user_thread);

  vm_map_entry_t *entry1 =
    vm_map_add_entry(get_kernel_vm_map(), KERNEL_START, KERNEL_END,
                     VM_PROT_READ | VM_PROT_WRITE);
  entry1->object = default_pager->pgr_alloc();

  t3->td_uspace = vm_map_new();
  vm_map_entry_t *entry2 = vm_map_add_entry(t3->td_uspace, USER_START, USER_END,
                                            VM_PROT_READ | VM_PROT_WRITE);
  entry2->object = default_pager->pgr_alloc();

  t4->td_uspace = vm_map_new();
  vm_map_entry_t *entry3 = vm_map_add_entry(t4->td_uspace, USER_START, USER_END,
                                            VM_PROT_READ | VM_PROT_WRITE);
  entry3->object = default_pager->pgr_alloc();

  sched_add(t1);
  sched_add(t2);
  sched_add(t3);
  sched_add(t4);

  sched_run();
}
