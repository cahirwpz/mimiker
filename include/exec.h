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
  /* TODO: Environment */
} exec_args_t;


typedef struct exec_args_proper {
  /* Path to the executable. */
  const char *prog_name;
  /* Program arguments. These will get copied to the stack of the
   * starting process. */
  const void *blob;
  size_t written;
  /* TODO: Environment */
} exec_args_t_proper;


typedef const char **argv_t;
int copyinargptrs(char *blob, argv_t user_argv, size_t *argc_out);
int copyinargs(char *blob, argv_t user_argv, size_t *argc_out);

int do_exec(const exec_args_t *prog);
noreturn void run_program(const exec_args_t *prog);

#endif /* !_SYS_EXEC_H_ */
