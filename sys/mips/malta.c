#define KLOG KL_INIT
#include <sys/interrupt.h>
#include <mips/cpuinfo.h>
#include <mips/mips.h>
#include <mips/malta.h>
#include <mips/exception.h>
#include <mips/interrupt.h>
#include <mips/timer.h>
#include <mips/tlb.h>
#include <sys/klog.h>
#include <sys/console.h>
#include <sys/context.h>
#include <sys/kenv.h>
#include <sys/libkern.h>
#include <sys/thread.h>
#include <sys/vm_physmem.h>
#include <sys/kasan.h>

static const char *whitespaces = " \t";

static size_t count_tokens(const char *str) {
  size_t ntokens = 0;

  do {
    str += strspn(str, whitespaces);
    if (*str == '\0')
      return ntokens;
    str += strcspn(str, whitespaces);
    ntokens++;
  } while (true);
}

static char **extract_tokens(kstack_t *stk, const char *str, char **tokens_p) {
  do {
    str += strspn(str, whitespaces);
    if (*str == '\0')
      return tokens_p;
    size_t toklen = strcspn(str, whitespaces);
    /* copy the token to memory managed by the kernel */
    char *token = kstack_alloc(stk, toklen + 1);
    strlcpy(token, str, toklen + 1);
    *tokens_p++ = token;
    /* append extra empty token when you see "--" */
    if (toklen == 2 && strncmp("--", str, 2) == 0)
      *tokens_p++ = NULL;
    str += toklen;
  } while (true);
}

static char *make_pair(kstack_t *stk, char *key, char *value) {
  int arglen = strlen(key) + strlen(value) + 2;
  char *arg = kstack_alloc(stk, arglen * sizeof(char));
  strlcpy(arg, key, arglen);
  strlcat(arg, "=", arglen);
  strlcat(arg, value, arglen);
  return arg;
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
 *     setup_kenv(argc, argv, envp);
 *
 *   will set global variables as follows:
 *     kenvp={"mimiker.elf", "memsize=128MiB", "uart.speed=115200",
 *            "arg1", "arg2=foo", "init=/bin/sh", "arg3=foobar"};
 *     kinit={NULL, "baz"};
 */

void *board_stack(int argc, char **argv, char **envp, unsigned memsize) {
  assert(argc == 2);

  kstack_t *stk = &thread0.td_kstack;

  /* See thread_entry_setup for explanation. */
  thread0.td_uframe = kstack_alloc_s(stk, exc_frame_t);

  int ntokens = 0;
  for (int i = 0; i < argc; ++i)
    ntokens += count_tokens(argv[i]);
  for (char **pair = envp; *pair; pair += 2)
    ntokens++;

  /* Both _kenvp and _kinit are going to point to the same array.
   * Their contents will be separated by NULL. */
  char **kenvp = kstack_alloc(stk, (ntokens + 2) * sizeof(char *));
  char **kinit = NULL;

  char **tokens = kenvp;
  tokens = extract_tokens(stk, argv[0], tokens);
  for (char **pair = envp; *pair; pair += 2)
    *tokens++ = make_pair(stk, pair[0], pair[1]);
  tokens = extract_tokens(stk, argv[1], tokens);
  *tokens = NULL;

  kstack_fix_bottom(stk);

  /* Let's find "--".
   * After we set it to NULL it's going to become first element of _kinit */
  for (char **argp = kenvp; *argp; argp++) {
    if (strcmp("--", *argp) == 0) {
      *argp++ = NULL;
      kinit = argp;
      break;
    }
  }

  init_kenv(kenvp, kinit);

  return stk->stk_ptr;
}

intptr_t ramdisk_get_start(void) {
  return MIPS_KSEG0_TO_PHYS(kenv_get_ulong("rd_start"));
}

size_t ramdisk_get_size(void) {
  return align(kenv_get_ulong("rd_size"), PAGESIZE);
}

extern __boot_data void *_kernel_end_kseg0;

static void malta_physmem(void) {
  /* XXX: workaround - pmap_enter fails to physical page with address 0 */
  paddr_t ram_start = MALTA_PHYS_SDRAM_BASE + PAGESIZE;
  paddr_t ram_end = MALTA_PHYS_SDRAM_BASE + kenv_get_ulong("memsize");
  paddr_t kern_start = MIPS_KSEG0_TO_PHYS(__boot);
  paddr_t kern_end = MIPS_KSEG0_TO_PHYS(_kernel_end_kseg0);
  paddr_t rd_start = ramdisk_get_start();
  paddr_t rd_end = rd_start + ramdisk_get_size();

  vm_physseg_plug(ram_start, kern_start);

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
  init_mips_intr();
  init_mips_timer();
  malta_physmem();
  intr_enable();
  kernel_init();
}
