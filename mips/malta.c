#define KLOG KL_INIT
#include <interrupt.h>
#include <physmem.h>
#include <malloc.h>
#include <mips/cpuinfo.h>
#include <mips/malta.h>
#include <mips/exc.h>
#include <mips/intr.h>
#include <mips/timer.h>
#include <mips/tlb.h>
#include <klog.h>
#include <kbss.h>
#include <console.h>
#include <pcpu.h>
#include <pmap.h>
#include <pool.h>
#include <stdc.h>
#include <sleepq.h>
#include <rman.h>
#include <thread.h>
#include <turnstile.h>
#include <vm_map.h>

extern int kernel_init(int argc, char **argv);

static struct {
  int argc;
  char **argv;
} _kenv;

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
 *     argc=3;
 *     argv={"test.elf", "arg1 arg2=val   arg3=foobar  ", "  init=/bin/sh "};
 *     envp={"memsize", "128MiB", "uart.speed", "115200"};
 *
 *   instruction:
 *     setup_kenv(argc, argv, envp);
 *
 *   will set global variable _kenv as follows:
 *     _kenv.argc=5;
 *     _kenv.argv={"test.elf", "arg1", "arg2=val", "arg3=foobar",
 *                 "init=/bin/sh", "memsize=128MiB", "uart.speed=115200"};
 */
static void setup_kenv(int argc, char **argv, char **envp) {
  unsigned ntokens = 0;

  for (int i = 0; i < argc; ++i)
    ntokens += count_tokens(argv[i]);
  for (char **pair = envp; *pair; pair += 2)
    ntokens++;

  _kenv.argc = ntokens;

  char **tokens = kbss_grow(ntokens * sizeof(char *));

  _kenv.argv = tokens;

  for (int i = 0; i < argc; ++i)
    tokens = extract_tokens(argv[i], tokens);
  for (char **pair = envp; *pair; pair += 2)
    *tokens++ = make_pair(pair[0], pair[1]);
}

char *kenv_get(const char *key) {
  unsigned n = strlen(key);

  for (int i = 1; i < _kenv.argc; i++) {
    char *arg = _kenv.argv[i];
    if ((strncmp(arg, key, n) == 0) && (arg[n] == '='))
      return arg + n + 1;
  }

  return NULL;
}

extern uint8_t __kernel_start[];

static void pm_bootstrap(unsigned memsize) {
  pm_init();

  pm_seg_t *seg = kbss_grow(pm_seg_space_needed(memsize));

  /*
   * Let's fix size of kernel bss section. We need to tell physical memory
   * allocator not to manage memory used by the kernel image along with all
   * memory allocated using \a kbss_grow.
   */
  void *__kernel_end = kbss_fix();

  /* create Malta physical memory segment */
  pm_seg_init(seg, MALTA_PHYS_SDRAM_BASE, MALTA_PHYS_SDRAM_BASE + memsize,
              MIPS_KSEG0_START);

  /* reserve kernel image and physical memory description space */
  pm_seg_reserve(seg, MIPS_KSEG0_TO_PHYS(__kernel_start),
                 MIPS_KSEG0_TO_PHYS(__kernel_end));

  pm_add_segment(seg);
}

static void thread_bootstrap(void) {
  /* Create main kernel thread */
  thread_t *td =
    thread_create("kernel-main", (void *)kernel_init, NULL, prio_uthread(255));

  exc_frame_t *kframe = td->td_kframe;
  kframe->a0 = (reg_t)_kenv.argc;
  kframe->a1 = (reg_t)_kenv.argv;
  kframe->sr |= SR_IE; /* the thread will run with interrupts enabled */
  td->td_state = TDS_RUNNING;
  PCPU_SET(curthread, td);
}

extern const uint8_t __malta_dtb_start[];

void platform_init(int argc, char **argv, char **envp, unsigned memsize) {
  kbss_init();

  setup_kenv(argc, argv, envp);
  cn_init();
  klog_init();
  pcpu_init();
  cpu_init();
  tlb_init();
  mips_intr_init();
  mips_timer_init();
  pm_bootstrap(memsize);
  pmap_init();
  pool_bootstrap();
  vm_map_init();
  kmem_bootstrap();
  sleepq_init();
  turnstile_init();
  thread_bootstrap();

  klog("Switching to 'kernel-main' thread...");
}
