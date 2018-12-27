#ifndef _SYS_EXEC_H_
#define _SYS_EXEC_H_

#ifdef _EXEC_IMPL
#include <common.h>
#include <elf/elf32.h>

typedef struct vnode vnode_t;
typedef struct proc proc_t;
typedef struct exec_args exec_args_t;

typedef int (*copy_path_t)(exec_args_t *args, char *path);
typedef int (*copy_ptr_t)(exec_args_t *args, char **ptr_p);
typedef int (*copy_str_t)(exec_args_t *args, char *str, size_t *copied_p);

struct exec_args {
  char *path; /* string of PATH_MAX length for executable path */

  char **argv; /* beginning of argv in the buffer */
  char **envv; /* beginning of envv in the buffer*/
  size_t argc;
  size_t envc;

  char *interp; /* interpreter path (or NULL) */

  copy_path_t copy_path;
  copy_ptr_t copy_ptr;
  copy_str_t copy_str;

  char *data;  /* buffer of ARG_MAX length for argv and envv data */
  char *end;   /* pointer to the end of data in the buffer */
  size_t left; /* space left in the buffer */
};

int exec_elf_inspect(vnode_t *vn, Elf32_Ehdr *eh);
int exec_elf_load(proc_t *p, vnode_t *vn, Elf32_Ehdr *eh);
int exec_shebang_inspect(vnode_t *vn);
int exec_shebang_load(vnode_t *vn, exec_args_t *args);

#endif /* !_EXEC_IMPL */

int do_execve(char *user_path, char *user_argv[], char *user_envp[]);
noreturn void run_program(char *path, char *argv[], char *envp[]);

#endif /* !_SYS_EXEC_H_ */
