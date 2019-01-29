#define KL_LOG KL_INIT
#include <klog.h>
#include <stdc.h>
#include <sched.h>
#include <thread.h>
#include <sysinit.h>
#include <vfs.h>

extern void kmain(void *);

/* This function mounts some initial filesystems. Normally this would be done by
   userspace init program. */
static void mount_fs(void) {
  do_mount(thread_self(), "initrd", "/");
  do_mount(thread_self(), "devfs", "/dev");
}

SYSINIT_ADD(mount_fs, mount_fs, DEPS("vfs"));

noreturn void kernel_init(void) {
  sysinit();

  klog("Kernel initialized!");

  thread_t *main_thread = thread_create("main", kmain, NULL, prio_kthread(0));
  sched_add(main_thread);

  sched_run();
}
