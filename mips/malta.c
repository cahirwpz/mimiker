#include <mips/malta.h>
#include <malloc.h>
#include <stdc.h>

void clear_bss() {
  extern unsigned int __bss[];
  extern unsigned int __ebss[];
  bzero(__bss, __ebss - __bss);
}

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
 *     argv={"<program name>", "arg1 arg2=val   arg3='foo bar'  "};
 *
 *   fixing:
 *     fix_argv(&argc, &argv);
 *
 *   after:
 *     argc=4;
 *     argv={"<program name>", "arg1", "arg2=val", "arg3=foo bar"};
 */

void fix_argv(int *p_argc, char ***p_argv) {
  char *cmd = (*p_argv)[1];
  char *s = cmd, *p = cmd;
  char quote = 0;
  int escape = 0;
  int argc;
  char **argv;

  if (*p_argc < 2)
    return;

  while(ISSPACE(*p)) ++p;
  for (argc = 1; *p; ++argc) {
    for (; *p; ++p) {
      if (escape) {
        escape = 0;
        *s++ = *p;
      }
      else if (*p == '\\') {
        escape = 1;
      }
      else if (*p == quote) {
        quote = 0;
      }
      else if (!quote && (*p == '\'' || *p == '"')) {
        quote = *p;
      }
      else if (quote || !ISSPACE(*p)) {
        *s++ = *p;
      }
      else {
        ++p;
        break;
      }
    }
    *s++ = 0;
    while(ISSPACE(*p)) ++p;
  }

  argv = kernel_sbrk(argc * sizeof(char *));
  argv[0] = (*p_argv)[0];

  p = cmd;
  for (int i = 1; i < argc; ++i) {
    argv[i] = p;
    while (*p++);
  }

  *p_argc = argc;
  *p_argv = argv;
}

void platform_init(int *p_argc, char ***p_argv) {
  clear_bss();
  fix_argv(p_argc, p_argv);
}
