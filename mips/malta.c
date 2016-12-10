#include <interrupt.h>
#include <malloc.h>
#include <mips/cpuinfo.h>
#include <mips/malta.h>
#include <mips/tlb.h>
#include <mips/uart_cbus.h>
#include <pcpu.h>
#include <stdc.h>
#include <thread.h>
#include <string.h>

extern unsigned int __bss[];
extern unsigned int __ebss[];
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
    char *token = kernel_sbrk(toklen + 1);
    strlcpy(token, str, toklen + 1);
    *tokens_p++ = token;
    str += toklen;
  } while (true);
}

static char *make_pair(char *key, char *value) {
  int arglen = strlen(key) + strlen(value) + 2;
  char *arg = kernel_sbrk(arglen * sizeof(char));
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

  char **tokens = kernel_sbrk(ntokens * sizeof(char *));

  _kenv.argv = tokens;

  for (int i = 0; i < argc; ++i)
    tokens = extract_tokens(argv[i], tokens);
  for (char **pair = envp; *pair; pair += 2)
    *tokens++ = make_pair(pair[0], pair[1]);
}

static void pm_bootstrap(unsigned memsize) {
  pm_init();

  /* Add Malta physical memory segment */
  pm_add_segment(MALTA_PHYS_SDRAM_BASE, MALTA_PHYS_SDRAM_BASE + memsize,
                 MIPS_KSEG0_START);
  /* shutdown sbrk allocator and remove used pages from physmem */
  pm_reserve(MALTA_PHYS_SDRAM_BASE,
             (pm_addr_t)kernel_sbrk_shutdown() - MIPS_KSEG0_START);
}

static void thread_bootstrap() {
  thread_init();

  /* Create main kernel thread */
  thread_t *td = thread_create("kernel-main", (void *)kernel_init, NULL);

  exc_frame_t *kframe = td->td_kframe;
  kframe->a0 = (reg_t)_kenv.argc;
  kframe->a1 = (reg_t)_kenv.argv;
  kframe->sr |= SR_IE; /* the thread will run with interrupts enabled */
  td->td_state = TDS_RUNNING;
  PCPU_SET(curthread, td);
}

void platform_init(int argc, char **argv, char **envp, unsigned memsize) {
  /* clear BSS section */
  bzero(__bss, __ebss - __bss);

  setup_kenv(argc, argv, envp);

  uart_init();
  pcpu_init();
  cpu_init();
  tlb_init();
  intr_init();
  pm_bootstrap(memsize);
  thread_bootstrap();

  kprintf("[startup] Switching to 'kernel-main' thread...\n");
}
