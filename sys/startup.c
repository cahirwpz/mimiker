#define KL_LOG KL_INIT
#include <klog.h>
#include <stdc.h>
#include <sched.h>
#include <thread.h>
#include <sysinit.h>
#include <vfs.h>
#include <interrupt.h>

extern void kmain(void *);

/* This function mounts some initial filesystems. Normally this would be done by
   userspace init program. */
static void mount_fs(void) {
  do_mount(thread_self(), "initrd", "/");
  do_mount(thread_self(), "devfs", "/dev");
}

SYSINIT_ADD(mount_fs, mount_fs, DEPS("vfs"));

int kernel_init(int argc, char **argv) {
  kprintf("Kernel arguments (%d): ", argc);
  for (int i = 0; i < argc; i++)
    kprintf("%s ", argv[i]);
  kprintf("\n");

  sysinit();
  klog("Kernel initialized!");

  thread_t *main_thread = thread_create("main", kmain, NULL, prio_kthread(0));
  sched_add(main_thread);

  thread_t *interrupt_thread =
    thread_create("interrupt", intr_thread, NULL, prio_ithread(0));
  sched_add(interrupt_thread);

  sched_run();
}
