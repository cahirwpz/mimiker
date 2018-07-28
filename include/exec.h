#ifndef _SYS_EXEC_H_
#define _SYS_EXEC_H_

#include <common.h>

typedef struct exec_args {
  /* Path to the executable. */
  const char *prog_name;
  /* Program arguments. These will get copied to the stack of the
   * starting process. */
  unsigned argc;
  const char **argv;
  unsigned envc;
  const char **envp;
  /* TODO: Environment */
} exec_args_t;

int do_exec(const exec_args_t *prog);
noreturn void run_program(const exec_args_t *prog);

#endif /* !_SYS_EXEC_H_ */
