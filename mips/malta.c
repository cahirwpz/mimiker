#include <mips/malta.h>
#include <malloc.h>
#include <string.h>

extern unsigned int __bss[];
extern unsigned int __ebss[];

unsigned _memsize;

struct {
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

void platform_init(int argc, char **argv, char **envp, unsigned memsize) {
  /* clear BSS section */
  bzero(__bss, __ebss - __bss);

  _memsize = memsize;

  setup_kenv(argc, argv, envp);
}
