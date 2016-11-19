#include <mips/malta.h>
#include <malloc.h>

void clear_bss() {
  extern unsigned int __bss[];
  extern unsigned int __ebss[];
  for (unsigned int *p = __bss, *end = __ebss; p < end; ++p) *p = 0;
}

#define ISSPACE(a) ((a) == ' ' || ((a) >= '\t' && (a) <= '\r'))

void fix_argv(int *p_argc, char ***p_argv) {
    char *cmd = (*p_argv)[1];
    char *s = cmd, *p = cmd;
    char quote = 0;
    int escape = 0;
    int argc;
    char **argv;

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
