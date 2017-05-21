#define KL_LOG KL_INIT
#include <klog.h>
#include <stdc.h>
#include <sched.h>
#include <thread.h>
#include <sysinit.h>
#include <vfs.h>

extern void main(void *);

/* This function mounts some initial filesystems. Normally this would be done by
   userspace init program. */
static void mount_fs() {
  do_mount(thread_self(), "initrd", "/");
  do_mount(thread_self(), "devfs", "/dev");
}

int kernel_init(int argc, char **argv) {
  kprintf("Kernel arguments (%d): ", argc);
  for (int i = 0; i < argc; i++)
    kprintf("%s ", argv[i]);
  kprintf("\n");

  sysinit();
  klog("Kernel initialized!");

  mount_fs();

  thread_t *main_thread = thread_create("main", main, NULL);
  sched_add(main_thread);

  sched_run();
}
