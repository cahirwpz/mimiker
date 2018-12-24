#ifndef _SYS_EXEC_H_
#define _SYS_EXEC_H_

#include <common.h>
#include <elf/elf32.h>

typedef struct vnode vnode_t;
typedef struct proc proc_t;

int exec_elf_inspect(vnode_t *vn, Elf32_Ehdr *eh);
int exec_elf_load(proc_t *p, vnode_t *vn, Elf32_Ehdr *eh);
int exec_shebang_inspect(vnode_t *vn);
int exec_shebang_interp(vnode_t *vn, char **interp_p);

int do_execve(char *user_path, char *user_argv[], char *user_envp[]);
noreturn void run_program(char *path, char *argv[], char *envp[]);

#endif /* !_SYS_EXEC_H_ */
