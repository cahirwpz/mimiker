#define KL_LOG KL_INIT
#include <sys/klog.h>
#include <sys/libkern.h>
#include <sys/kmem.h>
#include <sys/vmem.h>
#include <sys/pool.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/sched.h>
#include <sys/interrupt.h>
#include <sys/sleepq.h>
#include <sys/turnstile.h>
#include <sys/thread.h>
#include <sys/proc.h>
#include <sys/vfs.h>
#include <sys/vm_map.h>
#include <sys/vm_physmem.h>
#include <sys/pmap.h>
#include <sys/console.h>

extern void kmain(void *);

/* This function mounts some initial filesystems. Normally this would be done by
   userspace init program. */
static void mount_fs(void) {
  do_mount("initrd", "/");
  do_mount("devfs", "/dev");
  do_mount("tmpfs", "/tmp");
}

__noreturn void kernel_init(void) {
  init_pmap();
  init_vm_page();
  init_pool();
  init_vmem();
  init_kmem();
  init_kmalloc();
  init_vm_map();

  init_cons();

  /* Make dispatcher & scheduler structures ready for use. */
  init_sleepq();
  init_turnstile();
  init_sched();
  init_thread0();

  /* With scheduler ready we can create necessary threads. */
  init_ithreads();
  init_callout();
  preempt_enable();

  /* Init VFS and mount filesystems (including devfs). */
  init_vfs();
  mount_fs();

  /* First (FTTB also last) stage of device init. */
  init_devices();

  /* Some clocks has been found during device init process,
   * so it's high time to start system clock. */
  init_clock();

  init_proc();

  klog("Kernel initialized!");

  thread_t *main_thread = thread_create("main", kmain, NULL, prio_kthread(0));
  sched_add(main_thread);

  sched_run();
}
