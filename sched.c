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

static struct { intptr_t start, end; } range[] = {
  { 0xd0000000, 0xd0001000 },
  {  0x1000000,  0x1001000 },
  {  0x2000000,  0x2001000 }
};

void test_thread(volatile int *ptr) {
  kprintf("ptr: %p\n", ptr);
  while (1) {
    *ptr = *ptr + 1;
    kprintf("thread: %s, val: %d\n", thread_self()->td_name, *ptr);
    sched_yield();
  }
}

void main() {
  thread_t *t1 = thread_create("kernel-thread-1", test_thread);
  thread_t *t3 = thread_create("user-thread-1", test_thread);
  thread_t *t4 = thread_create("user-thread-2", test_thread);

  /* TODO: How to initialize thread main function arguments in machine
   * independent way? */
  t1->td_kframe->a0 = range[0].start;
  t3->td_kframe->a0 = range[1].start;
  t4->td_kframe->a0 = range[2].start;

  vm_map_entry_t *entry1 =
    vm_map_add_entry(get_kernel_vm_map(), range[0].start, range[0].end,
                     VM_PROT_READ | VM_PROT_WRITE);
  entry1->object = default_pager->pgr_alloc();

  t3->td_uspace = vm_map_new();
  vm_map_entry_t *entry2 =
    vm_map_add_entry(t3->td_uspace, range[1].start, range[1].end,
                     VM_PROT_READ | VM_PROT_WRITE);
  entry2->object = default_pager->pgr_alloc();

  t4->td_uspace = vm_map_new();
  vm_map_entry_t *entry3 =
    vm_map_add_entry(t4->td_uspace, range[2].start, range[2].end,
                     VM_PROT_READ | VM_PROT_WRITE);
  entry3->object = default_pager->pgr_alloc();

  vm_map_dump(get_kernel_vm_map());
  vm_map_dump(t3->td_uspace);
  vm_map_dump(t4->td_uspace);

  sched_add(t1);
  sched_add(t3);
  sched_add(t4);

  sched_run();
}
