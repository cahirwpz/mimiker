#include <common.h>
#include <stdc.h>
#include <mips/clock.h>
#include <malloc.h>
#include <pci.h>
#include <pmap.h>
#include <callout.h>
#include <sched.h>
#include <sleepq.h>
#include <taskqueue.h>
#include <thread.h>
#include <vm_object.h>
#include <vm_map.h>
#include <filedesc.h>
#include <vnode.h>
#include <mount.h>
#include <devfs.h>
#include <initrd.h>

extern void main(void *);

int kernel_init(int argc, char **argv) {
  kprintf("Kernel arguments (%d): ", argc);
  for (int i = 0; i < argc; i++)
    kprintf("%s ", argv[i]);
  kprintf("\n");

  pci_init();
  callout_init();
  pmap_init();
  vm_object_init();
  vm_map_init();
  sched_init();
  workqueue_init();
  mips_clock_init();

  ramdisk_init();
  vnode_init();
  vfs_init();
  file_init();
  fd_init();

  kprintf("[startup] kernel initialized\n");

  thread_t *main_thread = thread_create("main", main, NULL);
  sched_add(main_thread);

  sched_run();
}
