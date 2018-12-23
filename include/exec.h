#ifndef _SYS_EXEC_H_
#define _SYS_EXEC_H_

#include <common.h>
#include <elf/elf32.h>

typedef struct vnode vnode_t;
typedef struct proc proc_t;

typedef struct exec_args {
  /* Path to the executable. */
  char *prog_name;
  /* Program arguments and initial environment variables.
   * These will get copied to the stack of the starting process. */
  char **argv;
  char **envp;
  /* Space for all of the above objects.*/
  char data[0];
} exec_args_t;

#define EXEC_ARGS_SIZE (sizeof(exec_args_t) + ARG_MAX + PATH_MAX)

int exec_args_copyin(exec_args_t *exec_args, char *user_path, char **user_argv,
                     char **user_envp);
int exec_elf_inspect(vnode_t *vn, Elf32_Ehdr *eh);
int exec_elf_load(proc_t *p, vnode_t *vn, Elf32_Ehdr *eh);

int do_exec(const exec_args_t *prog);
noreturn void run_program(const exec_args_t *prog);

#endif /* !_SYS_EXEC_H_ */
