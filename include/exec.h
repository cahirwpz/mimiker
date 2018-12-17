#ifndef _SYS_EXEC_H_
#define _SYS_EXEC_H_

#include <common.h>

typedef struct exec_args {
  /* Path to the executable. */
  const char *prog_name;
#if 0
  /* Program arguments and initial environment variables.
   * These will get copied to the stack of the starting process. */
  const char **argv;
  const char **envp;
#endif
  /* Initial stack of the starting process. */
  int8_t *stack_image;
  size_t stack_size;
} exec_args_t;

typedef const char **argv_t;

int do_exec(const exec_args_t *prog);
noreturn void run_program(const exec_args_t *prog);

#endif /* !_SYS_EXEC_H_ */
