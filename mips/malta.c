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

/* 
 * For some reason arguments passed to kernel are not correctly splitted 
 * in argv array - in our case all arguments are stored in one string argv[1],
 * but we want to keep every argument in separate argv entry. This function
 * tokenize all argv strings and store every token into individual entry of 
 * new array. Also argc is corrected.
 *
 * Example:
 *
 *   before:
 *     argc=3;
 *     argv={"test.elf", "arg1 arg2=val   arg3=foobar  ", "  init=/bin/sh "};
 *
 *   fixing:
 *     fix_argv(&argc, &argv);
 *
 *   after:
 *     argc=5;
 *     argv={"test.elf", "arg1", "arg2=val", "arg3=foobar", "init=/bin/sh"};
 */
void fix_argv(int *p_argc, char ***p_argv) {
  int ntokens = 0;
  char **tokens, **p;

  for (int i = 0; i < *p_argc; ++i)
    ntokens += tokenize((*p_argv)[i], NULL);

  p = tokens = kernel_sbrk(ntokens * sizeof(char *));

  for (int i = 0; i < *p_argc; ++i)
    p += tokenize((*p_argv)[i], p);
   
  *p_argc = ntokens;
  *p_argv = tokens;
}

void platform_init(int argc, char **argv, char **envp, unsigned memsize) {
  /* clear BSS section */
  bzero(__bss, __ebss - __bss);
  
  _memsize = memsize;

  fix_argv(&argc, &argv);
  _kenv.argc = argc;
  _kenv.argv = argv;
}
