#include <common.h>
#include <stdc.h>
#include <mips/malta.h>
#include <mips/cpuinfo.h>
#include <mips/uart_cbus.h>
#include <mips/tlb.h>
#include <mips/clock.h>
#include <interrupt.h>
#include <pcpu.h>
#include <malloc.h>
#include <physmem.h>
#include <pci.h>
#include <pmap.h>
#include <callout.h>
#include <sched.h>
#include <sleepq.h>
#include <thread.h>
#include <vm_object.h>
#include <vm_map.h>

extern int main(int argc, char **argv);

int kernel_boot(int argc, char **argv) {
  uart_init();

  kprintf("Kernel arguments (%d): ", argc);
  for (int i = 0; i < argc; i++)
    kprintf("%s ", argv[i]);
  kprintf("\n");

  cpu_init();
  pcpu_init();
  pci_init();
  pm_init();
  intr_init();
  callout_init();
  tlb_init();
  pmap_init();
  vm_object_init();
  vm_map_init();
  sched_init();
  sleepq_init();
  mips_clock_init();
  kprintf("[startup] subsystems initialized\n");
  thread_init((void (*)())main, 2, argc, argv);
}
