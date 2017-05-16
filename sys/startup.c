#include <common.h>
#include <stdc.h>
#include <mips/clock.h>
#include <malloc.h>
#include <pci.h>
#include <pmap.h>
#include <callout.h>
#include <sched.h>
#include <sleepq.h>
#include <thread.h>
#include <vm_object.h>
#include <vm_map.h>
#include <filedesc.h>
#include <vnode.h>
#include <mount.h>
#include <devfs.h>
#include <initrd.h>
#include <device.h>
#include <proc.h>
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

  callout_init();
  pmap_init();
  vm_object_init();
  vm_map_init();
  sched_init();
  mips_clock_init();

  ramdisk_init();
  vnode_init();
  vfs_init();
  file_init();
  fd_init();
  proc_init();

  driver_init();
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
