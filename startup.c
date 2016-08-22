#include <common.h>
#include <stdc.h>
#include <mips/cpuinfo.h>
#include <mips/uart_cbus.h>
#include <mips/tlb.h>
#include <interrupts.h>
#include <clock.h>
#include <malloc.h>
#include <physmem.h>
#include <rtc.h>
#include <pci.h>
#include <pmap.h>
#include <callout.h>
#include <thread.h>
#include <vm_object.h>
#include <vm_map.h>

extern int main(int argc, char **argv, char **envp);

int kernel_boot(int argc, char **argv, char **envp) {
  uart_init();

  kprintf("Kernel arguments: ");
  for (int i = 0; i < argc; i++)
    kprintf("%s ", argv[i]);
  kprintf("\n");

  kprintf("Kernel environment: ");
  char **_envp = envp;
  while (*_envp) {
    char *key = *_envp++;
    char *val = *_envp++;
    kprintf("%s=%s ", key, val);
  }
  kprintf("\n");

  cpu_init();
  pci_init();
  pm_init();
  intr_init();
  clock_init();
  callout_init();
  rtc_init();
  tlb_init();
  vm_object_init();
  vm_map_init();
  kprintf("[startup] subsystems initialized\n");
  thread_init((void (*)())main, 3, argc, argv, envp);
}
