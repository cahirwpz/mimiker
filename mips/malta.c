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

/* 
 * This function works almost like strtok, but it counts tokens in string and 
 * stores them into given array. If array is NULL, then function only counts 
 * tokens without modifying string. 
 */
int tokenize(char *str, char **tokens) {
  int ntokens;
  const char *whitespaces = " \t";

  str += strspn(str, whitespaces); // trim left whitespaces
  for (ntokens = 0; *str; ++ntokens) {
    char *token = str;
    int tokenlen = strcspn(str, whitespaces);
    str += tokenlen;  // jump over the token
    str += strspn(str, whitespaces);  // trim left whitespaces
    if (tokens) {
      // null-terminate token and store it in array:
      token[tokenlen] = 0;
      tokens[ntokens] = token;
    }
  }

  return ntokens;
}

// Creates new string that is concatenation of given two separated by '='.
char *env2arg(char *key, char *value) {
    int keylen = strlen(key);
    int valuelen = strlen(value);
    int arglen = keylen + 1 + valuelen + 1;
    char *arg = kernel_sbrk(arglen * sizeof(char));
    strlcpy(arg, key, arglen);
    arg[keylen] = '=';
    strlcpy(arg + keylen + 1, value, arglen - keylen - 1);
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
void setup_kenv(int argc, char **argv, char **envp) {
  int ntokens = 0;
  char **tokens, **p;

  for (int i = 0; i < argc; ++i)
    ntokens += tokenize(argv[i], NULL);
  for (char **key = envp; *key; key += 2)
    ntokens += 1;

  p = tokens = kernel_sbrk(ntokens * sizeof(char *));

  for (int i = 0; i < argc; ++i)
    p += tokenize(argv[i], p);
  for (char **pair = envp; *pair; pair += 2)
    *p++ = env2arg(pair[0], pair[1]);
   
  _kenv.argc = ntokens;
  _kenv.argv = tokens;
}

void platform_init(int argc, char **argv, char **envp, unsigned memsize) {
  /* clear BSS section */
  bzero(__bss, __ebss - __bss);
  
  _memsize = memsize;

  setup_kenv(argc, argv, envp);
}
