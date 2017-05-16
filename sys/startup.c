#include <stdc.h>
#include <sched.h>
#include <thread.h>
#include <sysinit.h>
#include <vfs.h>

extern void main(void *);

static void mount_fs();

int kernel_init(int argc, char **argv) {
  kprintf("Kernel arguments (%d): ", argc);
  for (int i = 0; i < argc; i++)
    kprintf("%s ", argv[i]);
  kprintf("\n");

  sysinit_sort();

  mount_fs();

  kprintf("[startup] kernel initialized\n");

  thread_t *main_thread = thread_create("main", main, NULL);
  sched_add(main_thread);

  sched_run();
}

/* This function mounts some initial filesystems. Normally this would be done by
   userspace init program. */
static void mount_fs() {
  do_mount(thread_self(), "initrd", "/");
  do_mount(thread_self(), "devfs", "/dev");
}
