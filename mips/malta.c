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
#include <syslimits.h>

extern int kernel_init(int argc, char **argv);

static struct {
  int argc;
  char **argv;
  char **user_argv;
  char **user_envv;
} _kenv;

static const char *whitespace = " \t";
static const char *token_separator = " \t\"";
static const char *quot_str = "\"";
static const char quot_char = '\"';
/*
  <token>        ::= <token_prefix>[<token_suffix>]
  <token_prefix> ::= any sequence of characters distinct
                   than whitespace or quot_char
  <token_suffix> ::= <quot_char>any sequence of characters distinct
                   than quot_char<quot_char>
  <token_sequence> ::=
  <whitespace>*[<token><whitespace>*(<whitespace><token>(<whitespace>)*)*]
*/

static size_t token_size(const char *input) {
  size_t len = strcspn(input, token_separator);
  if (input[len] == quot_char)
    len += strcspn(input + len + 1, quot_str) + 2;
  return len;
}

static char **extract_tokens(char *seq) {
  unsigned ntokens = 0;
  for (char *p = seq; *(p += strspn(p, whitespace)); p += token_size(p))
    ntokens++;
  char **tokens = kbss_grow((ntokens + 1) * sizeof(char *)), **ret = tokens;
  size_t len1;
  for (char *p = seq; *(p += strspn(p, whitespace)); p += len1 - 1) {
    len1 = token_size(p) + 1;
    *tokens = kbss_grow(len1 * sizeof(char));
    strlcpy(*tokens++, p, len1);
  }
  return ret;
}

static char **extract_qtd_tokens(char *qseq) {
  size_t len = strcspn(++qseq, quot_str);
  qseq[len] = '\0';
  char **ret = extract_tokens(qseq);
  qseq[len] = quot_char;
  return ret;
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

static void setup_kenv(int argc, char **argv) {
  size_t args_len = 1;
  for (int i = 0; i < argc; i++)
    args_len += strlen(argv[i]) + 1;

  char *args_seq = kbss_grow(args_len * sizeof(char)), *p = args_seq;
  for (int i = 0; i < argc; i++)
    p = p + strlcpy(p, argv[i], ARG_MAX), *p++ = *whitespace;

  _kenv.argv = extract_tokens(args_seq);
  while (*(_kenv.argv + _kenv.argc))
    _kenv.argc++;

  if ((p = kenv_get("init")))
    _kenv.user_argv = extract_qtd_tokens(p);
  _kenv.user_envv =
    (p = kenv_get("envv")) ? extract_qtd_tokens(p) : (char *[]){NULL};
}

char **kenv_get_user_argv(void) {
  return _kenv.user_argv;
}

char **kenv_get_user_envv(void) {
  return _kenv.user_envv;
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

void platform_init(int argc, char **argv, char **envp, unsigned memsize) {
  kbss_init();

  setup_kenv(argc, argv);
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
