#define KL_LOG KL_INIT
#include <sys/klog.h>
#include <sys/libkern.h>
#include <sys/sched.h>
#include <sys/thread.h>
#include <sys/sysinit.h>
#include <sys/vfs.h>

extern void kmain(void *);

/* This function mounts some initial filesystems. Normally this would be done by
   userspace init program. */
static void mount_fs(void) {
  do_mount("initrd", "/");
  do_mount("devfs", "/dev");
  do_mount("tmpfs", "/tmp");
}

SYSINIT_ADD(mount_fs, mount_fs, DEPS("vfs"));

__noreturn void kernel_init(void) {
  sysinit();
  klog("Kernel initialized!");

  thread_t *main_thread = thread_create("main", kmain, NULL, prio_kthread(0));
  sched_add(main_thread);

  sched_run();
}
