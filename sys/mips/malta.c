#define KLOG KL_INIT
#include <sys/interrupt.h>
#include <mips/cpuinfo.h>
#include <mips/mips.h>
#include <mips/malta.h>
#include <mips/context.h>
#include <mips/interrupt.h>
#include <mips/tlb.h>
#include <sys/klog.h>
#include <sys/console.h>
#include <sys/context.h>
#include <sys/cmdline.h>
#include <sys/kenv.h>
#include <sys/libkern.h>
#include <sys/initrd.h>
#include <sys/thread.h>
#include <sys/vm_physmem.h>
#include <sys/kasan.h>

static char *make_pair(kstack_t *stk, char *key, char *value) {
  int arglen = strlen(key) + strlen(value) + 2;
  char *arg = kstack_alloc(stk, arglen * sizeof(char));
  strlcpy(arg, key, arglen);
  strlcat(arg, "=", arglen);
  strlcat(arg, value, arglen);
  return arg;
}

static int count_args(int argc, char **argv, char **envp) {
  int ntokens = 0;
  for (int i = 0; i < argc; ++i)
    ntokens += cmdline_count_tokens(argv[i]);
  for (char **pair = envp; *pair; pair += 2)
    ntokens++;
  return ntokens;
}

static void process_args(char **argv, char **envp, char **tokens,
                         kstack_t *stk) {
  tokens = cmdline_extract_tokens(stk, argv[0], tokens);
  for (char **pair = envp; *pair; pair += 2)
    *tokens++ = make_pair(stk, pair[0], pair[1]);
  tokens = cmdline_extract_tokens(stk, argv[1], tokens);
  *tokens = NULL;
}

/*
 * For some reason arguments passed to kernel are not correctly splitted
 * in argv array - in our case all arguments are stored in one string argv[1],
 * but we want to keep every argument in separate argv entry. This function
 * tokenize all argv strings and store every single token into individual entry
 * of new array.
 *
 * For our needs we also convert passed environment variables and put them
 * into new argv array.
 *
 * Example:
 *
 *   For arguments:
 *     argc=2;
 *     argv={"mimiker.elf", "arg1 arg2=foo init=/bin/sh arg3=bar -- baz"};
 *     envp={"memsize", "128MiB", "uart.speed", "115200"};
 *
 *   instruction:
 *     board_stack(argc, argv, envp);
 *
 *   will set global variables as follows:
 *     kenvp={"mimiker.elf", "memsize=128MiB", "uart.speed=115200",
 *            "arg1", "arg2=foo", "init=/bin/sh", "arg3=foobar"};
 *     kinit={NULL, "baz"};
 *
 *   Please note that both kenvp and kinit point to the same array.
 *   Their contents will be separated by NULL.
 */
void *board_stack(int argc, char **argv, char **envp) {
  assert(argc == 2);

  kstack_t *stk = &thread0.td_kstack;

  /* See thread_entry_setup for explanation. */
  thread0.td_uctx = kstack_alloc_s(stk, user_ctx_t);

  int ntokens = count_args(argc, argv, envp);
  char **kenvp = kstack_alloc(stk, (ntokens + 2) * sizeof(char *));
  process_args(argv, envp, kenvp, stk);
  kstack_fix_bottom(stk);
  init_kenv(kenvp);

  /* Move ramdisk start address to physical memory. */
  char *s = kenv_get("rd_start");
  long addr = MIPS_KSEG0_TO_PHYS(kenv_get_ulong("rd_start"));
  snprintf(s, strlen(s), "0x%lx", addr);

  return stk->stk_ptr;
}

static void malta_physmem(void) {
  /* XXX: workaround - pmap_enter fails to physical page with address 0 */
  paddr_t ram_start = MALTA_PHYS_SDRAM_BASE + PAGESIZE;
  paddr_t ram_end = MALTA_PHYS_SDRAM_BASE + kenv_get_ulong("memsize");
  paddr_t kern_start = MIPS_KSEG0_TO_PHYS(__boot);
  paddr_t kern_end = MIPS_KSEG0_TO_PHYS(_bootmem_end);
  paddr_t rd_start = ramdisk_get_start();
  paddr_t rd_end = rd_start + ramdisk_get_size();

  vm_physseg_plug(ram_start, kern_start);
  vm_physseg_plug_used(kern_start, kern_end);

  if (rd_start != rd_end) {
    vm_physseg_plug(kern_end, rd_start);
    vm_physseg_plug_used(rd_start, rd_end);
    vm_physseg_plug(rd_end, ram_end);
  } else {
    vm_physseg_plug(kern_end, ram_end);
  }
}

__noreturn void board_init(void) {
  init_kasan();
  init_klog();
  init_mips_cpu();
  init_mips_tlb();
  malta_physmem();
  intr_enable();
  kernel_init();
}
