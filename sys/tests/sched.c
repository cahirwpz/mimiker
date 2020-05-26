#include <sys/libkern.h>
#include <sys/time.h>
#include <sys/thread.h>
#include <sys/sched.h>
#include <sys/vm_map.h>
#include <sys/vm_pager.h>
#include <sys/ktest.h>

#if 0
static void demo_thread_1(void) {
  while (true) {
    timespec_t start = nanoouptime();
    kprintf("[%8zu] Running '%s' thread.\n", (size_t)ts2st(start),
            thread_self()->td_name);
    timespec_t now = nanoouptime();
    while (ts2st(now) < ts2st(start) + 20)
      now = nanoouptime();
  }
}

static void demo_thread_2(void) {
  kprintf("Running '%s' thread. Let's yield!\n", thread_self()->td_name);
  sched_yield();
  demo_thread_1();
}

void main(void) {
  thread_t *t1 = thread_create("t1", demo_thread_1, NULL);
  thread_t *t2 = thread_create("t2", demo_thread_1, NULL);
  thread_t *t3 = thread_create("t3", demo_thread_1, NULL);
  thread_t *t4 = thread_create("t4", demo_thread_2, NULL);
  thread_t *t5 = thread_create("t5", demo_thread_2, NULL);

  sched_add(t1);
  sched_add(t2);
  sched_add(t3);
  sched_add(t4);
  sched_add(t5);

  sched_run();
}
#endif

#if 0
static struct {
  intptr_t start, end;
} range[] = {
  {0xd0000000, 0xd0001000}, {0x1000000, 0x1001000}, {0x2000000, 0x2001000}};

static void test_thread(void *p) {
  int *ptr = p;
  kprintf("ptr: %p\n", ptr);
  while (1) {
    *ptr = *ptr + 1;
    kprintf("thread: %s, val: %d\n", thread_self()->td_name, *ptr);
    sched_yield();
  }
}

static int test_sched(void) {
  thread_t *t1 =
    thread_create("kernel-thread-1", test_thread, (void *)range[0].start);
  thread_t *t3 =
    thread_create("user-thread-1", test_thread, (void *)range[1].start);
  thread_t *t4 =
    thread_create("user-thread-2", test_thread, (void *)range[2].start);

  vm_map_entry_t *entry1 =
    vm_map_add_entry(get_kernel_vm_map(), range[0].start, range[0].end,
                     VM_PROT_READ | VM_PROT_WRITE);
  entry1->object = default_pager->pgr_alloc();

  t3->td_uspace = vm_map_new();
  vm_map_entry_t *entry2 = vm_map_add_entry(
    t3->td_uspace, range[1].start, range[1].end, VM_PROT_READ | VM_PROT_WRITE);
  entry2->object = default_pager->pgr_alloc();

  t4->td_uspace = vm_map_new();
  vm_map_entry_t *entry3 = vm_map_add_entry(
    t4->td_uspace, range[2].start, range[2].end, VM_PROT_READ | VM_PROT_WRITE);
  entry3->object = default_pager->pgr_alloc();

  vm_map_dump(get_kernel_vm_map());
  vm_map_dump(t3->td_uspace);
  vm_map_dump(t4->td_uspace);

  sched_add(t1);
  sched_add(t3);
  sched_add(t4);

  sched_run();
  return KTEST_FAILURE;
}

KTEST_ADD(sched, test_sched, KTEST_FLAG_NORETURN);
#endif
