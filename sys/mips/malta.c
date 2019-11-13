#define KLOG KL_INIT
#include <sys/interrupt.h>
#include <sys/physmem.h>
#include <sys/malloc.h>
#include <mips/cpuinfo.h>
#include <mips/malta.h>
#include <mips/exc.h>
#include <mips/intr.h>
#include <mips/timer.h>
#include <mips/tlb.h>
#include <sys/klog.h>
#include <sys/kbss.h>
#include <sys/console.h>
#include <sys/context.h>
#include <sys/pcpu.h>
#include <sys/pmap.h>
#include <sys/pool.h>
#include <sys/libkern.h>
#include <sys/thread.h>
#include <sys/vm_map.h>

extern int kernel_init(void);

static char **_kenvp;
static char **_kinit = (char *[2]){ NULL, NULL };

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

static char **extract_tokens(const char *str, char **tokens_p) {
  do {
    str += strspn(str, whitespaces);
    if (*str == '\0')
      return tokens_p;
    size_t toklen = strcspn(str, whitespaces);
    /* copy the token to memory managed by the kernel */
    char *token = kbss_grow(toklen + 1);
    strlcpy(token, str, toklen + 1);
    *tokens_p++ = token;
    /* append extra empty token when you see "--" */
    if (toklen == 2 && strncmp("--", str, 2) == 0)
      *tokens_p++ = NULL;
    str += toklen;
  } while (true);
}

static char *make_pair(char *key, char *value) {
  int arglen = strlen(key) + strlen(value) + 2;
  char *arg = kbss_grow(arglen * sizeof(char));
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
 *     _kenvp={"mimiker.elf", "memsize=128MiB", "uart.speed=115200",
 *             "arg1", "arg2=foo", "init=/bin/sh", "arg3=foobar"};
 *     _kinit={NULL, "baz"};
 */
static void setup_kenv(int argc, char **argv, char **envp) {
  int ntokens = 0;

  assert(argc == 2);

  for (int i = 0; i < argc; ++i)
    ntokens += count_tokens(argv[i]);
  for (char **pair = envp; *pair; pair += 2)
    ntokens++;

  /* Both _kenvp and _kinit are going to point to the same array.
   * Their contents will be separated by NULL. */
  _kenvp = kbss_grow((ntokens + 2) * sizeof(char *));

  char **tokens = _kenvp;
  tokens = extract_tokens(argv[0], tokens);
  for (char **pair = envp; *pair; pair += 2)
    *tokens++ = make_pair(pair[0], pair[1]);
  tokens = extract_tokens(argv[1], tokens);
  *tokens = NULL;

  /* Let's find "--".
   * After we set it to NULL it's going to become first element of _kinit */
  for (char **argp = _kenvp; *argp; argp++) {
    if (strcmp("--", *argp) == 0) {
      *argp++ = NULL;
      _kinit = argp;
      break;
    }
  }
}

char *kenv_get(const char *key) {
  unsigned n = strlen(key);

  for (char **argp = _kenvp; *argp; argp++) {
    char *arg = *argp;
    if ((strncmp(arg, key, n) == 0) && (arg[n] == '='))
      return arg + n + 1;
  }

  return NULL;
}

char **kenv_get_init(void) {
  _kinit[0] = kenv_get("init");
  return _kinit;
}

static intptr_t __rd_start;
static size_t __rd_size;

intptr_t ramdisk_get_start(void) {
  return __rd_start;
}

size_t ramdisk_get_size(void) {
  return __rd_size;
}

static void ramdisk_init(void) {
  char *s;
  size_t n;

  /* parse 'rd_start' */
  s = kenv_get("rd_start");

  /* rd_start: skip '0x' under qemu */
  if (s[0] == '0' && s[1] == 'x')
    s += 2;

  /* rd_start: skip 'ffffffff' under qemu */
  n = strlen(s);
  if (n > 8)
    s += n - 8;

  __rd_start = strtoul(s, NULL, 16);

  /* parse 'rd_size' */
  s = kenv_get("rd_size");

  __rd_size = align(strtoul(s, NULL, 10), PAGESIZE);
}

extern char __kernel_start[];
char *__kernel_end;

static void pm_bootstrap(unsigned memsize) {
  pm_init();

  pm_seg_t *seg = kbss_grow(pm_seg_space_needed(memsize));

  /*
   * Let's fix size of kernel bss section. We need to tell physical memory
   * allocator not to manage memory used by the kernel image along with all
   * memory allocated using \a kbss_grow.
   */
  __kernel_end = kbss_fix();

  /* create Malta physical memory segment */
  pm_seg_init(seg, MALTA_PHYS_SDRAM_BASE, MALTA_PHYS_SDRAM_BASE + memsize,
              MIPS_KSEG0_START);

  /* XXX: workaround - pmap_enter fails to physical page with address 0 */
  pm_seg_reserve(seg, MALTA_PHYS_SDRAM_BASE, MALTA_PHYS_SDRAM_BASE + PAGESIZE);

  /* reserve kernel image and physical memory description space */
  pm_seg_reserve(seg, MIPS_KSEG0_TO_PHYS(__kernel_start),
                 MIPS_KSEG0_TO_PHYS(__kernel_end));

  pm_add_segment(seg);
}

extern const uint8_t __malta_dtb_start[];

__noreturn void platform_init(int argc, char **argv, char **envp,
                              unsigned memsize) {
  setup_kenv(argc, argv, envp);
  cn_init();
  klog_init();
  pcpu_init();
  cpu_init();
  tlb_init();
  ramdisk_init();
  mips_intr_init();
  mips_timer_init();
  pm_bootstrap(memsize);
  pmap_bootstrap();
  pool_bootstrap();
  vm_map_bootstrap();
  kmem_bootstrap();
  thread_bootstrap();

  /* Set up main kernel thread. */
  thread_t *td = thread_self();
  /* The thread will run with interrupts enabled */
  td->td_kframe->sr |= SR_IE;

  klog("Switching to 'kernel-main' thread...");
  ctx_switch(NULL, td);
  __unreachable();
}
