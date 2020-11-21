#ifndef _SYS_EXEC_H_
#define _SYS_EXEC_H_

#ifdef _EXEC_IMPL
#include <sys/cdefs.h>
#include <machine/elf_machdep.h>
#include <sys/exec_elf.h>

typedef struct vnode vnode_t;
typedef struct proc proc_t;
typedef struct exec_args exec_args_t;

struct exec_args {
  char *path;   /* string of PATH_MAX length for executable path */
  char *interp; /* interpreter path (or NULL) */

  char **argv; /* beginning of argv in the buffer */
  char **envv; /* beginning of envv in the buffer*/
  size_t argc;
  size_t envc;

  char *data;  /* buffer of ARG_MAX length for argv and envv data */
  char *end;   /* pointer to the end of data in the buffer */
  size_t left; /* space left in the buffer */
};

int exec_elf_inspect(vnode_t *vn, Elf_Ehdr *eh);
int exec_elf_load(proc_t *p, vnode_t *vn, Elf_Ehdr *eh);
int exec_shebang_inspect(vnode_t *vn);
int exec_shebang_load(vnode_t *vn, exec_args_t *args);

#endif /* !_EXEC_IMPL */

int do_execve(const char *user_path, char *const *u_argp, char *const *u_envp);
__noreturn void kern_execve(const char *path, char *const *argv,
                            char *const *envp);

#endif /* !_SYS_EXEC_H_ */
