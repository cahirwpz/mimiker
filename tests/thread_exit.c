#include <stdc.h>
#include <clock.h>
#include <thread.h>
#include <sched.h>
#include <vm_map.h>
#include <vm_pager.h>

int exit_time[] = {100, 150, 200};
realtime_t start;

static struct {
  intptr_t start, end;
} range[] = {
  {0xd0000000, 0xd0001000}, {0x1000000, 0x1001000}, {0x2000000, 0x2001000}};

void test_thread(void *p) {
  int e = *(int *)p;
  while (1) {
    realtime_t tdiff = clock_get() - start;
    //kprintf("Running '%s' thread for about %zu.\n", thread_self()->td_name,
        //(size_t)tdiff);
    if (tdiff > e)
      thread_exit();
    else
      sched_yield();
  }
}

void main() {
  thread_t *t1 =
    thread_create("kernel-thread-1", test_thread, &exit_time[0]);
  thread_t *t2 =
    thread_create("kernel-thread-2", test_thread, &exit_time[1]);
  thread_t *t3 =
    thread_create("kernel-thread-3", test_thread, &exit_time[2]);

  vm_map_entry_t *entry1 =
    vm_map_add_entry(get_kernel_vm_map(), range[0].start, range[0].end,
                     VM_PROT_READ | VM_PROT_WRITE);
  entry1->object = default_pager->pgr_alloc();

  vm_map_dump(get_kernel_vm_map());

  start = clock_get();
  sched_add(t1);
  sched_add(t2);
  sched_add(t3);

  thread_dump_all();

  sched_run();
}
