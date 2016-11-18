#include <args.h>
#include <stdc.h>
#include <string.h>
#include <malloc.h>

int kernel_argc;
char **kernel_argv;

void kernel_args_parse(int argc, char **argv, char **envp) {

  /* First, calculate total length so that we can allocate memory where we will
   * copy kernel arguments.
   */
  size_t total_len = 0;
  for (unsigned int i = 0; i < argc; i++)
    total_len += 1 + strlen(argv[i]);
  /* Now correct argc, by counting spaces in each argument. */
  kernel_argc = argc;
  for (unsigned int i = 0; i < argc; i++)
    for (char *j = argv[i]; *j != '\0'; j++)
      if (*j == ' ')
        kernel_argc++;
  /* Allocate memory for arguments. We never free this memory. */
  char *args = kernel_sbrk(total_len);
  kernel_argv = kernel_sbrk(kernel_argc * sizeof(char *));
  /* Copy arguments and prepare values for kernel_argv. */
  char *curr = args;
  unsigned int i = 0, j = 0;
  for (; i < argc; i++) {
    char *tok = strsep(argv + i, " ");
    while (tok) {
      strcpy(curr, tok);
      kernel_argv[j++] = curr;
      curr += strlen(tok) + 1;
      tok = strsep(argv + i, " ");
    }
  }

  kprintf("Kernel arguments:\n");
  for (int i = 0; i < kernel_argc; i++)
    kprintf("  %s\n", kernel_argv[i]);

  kprintf("Kernel environment: ");
  char **_envp = envp;
  while (*_envp) {
    char *key = *_envp++;
    char *val = *_envp++;
    kprintf("%s=%s ", key, val);
  }
  kprintf("\n");
}

const char *kernel_args_get(const char *key) {
  int n = strlen(key);
  for (int i = 0; i < kernel_argc; i++)
    if (strncmp(key, kernel_argv[i], n) == 0)
      return kernel_argv[i] + n + 1;
  return NULL;
}

int kernel_args_get_flag(const char *flag) {
  for (int i = 0; i < kernel_argc; i++)
    if (strcmp(flag, kernel_argv[i]) == 0)
      return 1;
  return 0;
}
