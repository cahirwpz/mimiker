#ifndef _SYS_EXEC_H_
#define _SYS_EXEC_H_

#include <common.h>
#include <malloc.h>

typedef struct exec_args {
  /* Path to the executable. */
  const char *prog_name;
  /* Program arguments and initial environment variables.
   * These will get copied to the stack of the starting process. */
  char **argv;
  char **envp;
  char data[0];
} exec_args_t;

int exec_args_copyin(exec_args_t *exec_args, char *user_path, char **user_argv,
                     char **user_envp);

int do_exec(const exec_args_t *prog);
noreturn void run_program(const exec_args_t *prog);

#endif /* !_SYS_EXEC_H_ */
