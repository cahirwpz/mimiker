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

  sched_run();
}
