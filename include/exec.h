#ifndef _SYS_EXEC_H_
#define _SYS_EXEC_H_

#include <common.h>
#include <malloc.h>

typedef struct {
  size_t capacity;
  void *start;  
  void *end;
} blob_t;

blob_t blob_init(kmem_pool_t* pool, size_t capacity);
void blob_destroy(kmem_pool_t* pool, blob_t blob);


typedef struct exec_args {
  /* Path to the executable. */
  const char *prog_name;
  /* Program arguments and initial environment variables.
   * These will get copied to the stack of the starting process. */
  const char **argv;
  const char **envp;
  blob_t blob;
} exec_args_t;

int exec_args_copyin(exec_args_t *exec_args, vaddr_t user_path,
                     vaddr_t user_argv, vaddr_t user_envp);
void exec_args_destroy(exec_args_t *exec_args);

int do_exec(const exec_args_t *prog);
noreturn void run_program(const exec_args_t *prog);

#endif /* !_SYS_EXEC_H_ */
