#include <mips/malta.h>
#include <malloc.h>
#include <stdc.h>

extern unsigned int __bss[];
extern unsigned int __ebss[];

unsigned _memsize;

struct {
  int argc;
  char **argv;
} _kenv;

#if 0
#define ISSPACE(a) ((a) == ' ' || ((a) >= '\t' && (a) <= '\r'))

/*
 * For some reason arguments passed to the kernel are stored in one string -
 * it means that argc is always equals 2 and argv[1] string contains all passed
 * parameters. Because we want each parameter in separate argv entry
 * (as it should be) we need to fix it. This function parse that argv[1]
 * string, split it to separate arguments, creates new argv table with proper
 * entries and corrects argc.
 *
 * Example:
 *
 *   before:
 *     argc=2;
 *     argv={"<program name>", "arg1 arg2=val   arg3=foobar  "};
 *
 *   fixing:
 *     fix_argv(&argc, &argv);
 *
 *   after:
 *     argc=4;
 *     argv={"<program name>", "arg1", "arg2=val", "arg3=foobar"};
 */

static void fix_argv(int *p_argc, char ***p_argv) {
  if (*p_argc < 2)
    return;

  char *cmd = (*p_argv)[1];
  char *s = cmd, *p = cmd;
  int argc;
  char **argv;

  // argv[1] string is modified - all whitespaces are overwritten 
  // by non-whitespace characters and every parameter is terminated 
  // with null character
  while (ISSPACE(*p)) ++p;
  for (argc = 1; *p; ++argc) {
    while (*p && !ISSPACE(*p)) *s++ = *p++;
    while (ISSPACE(*p)) ++p;
    *s++ = 0;
  }

  argv = kernel_sbrk(argc * sizeof(char *));

  p = cmd;
  argv[0] = (*p_argv)[0];
  for (int i = 1; i < argc; ++i) {
    argv[i] = p;
    while (*p++);  // find first character of next parameter
  }

  *p_argc = argc;
  *p_argv = argv;
}
#endif

void platform_init(int argc, char **argv, char **envp, unsigned memsize) {
  /* clear BSS section */
  bzero(__bss, __ebss - __bss);
  
  _memsize = memsize;

  _kenv.argc = argc;
  _kenv.argv = argv;
}
