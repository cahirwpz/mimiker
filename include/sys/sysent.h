#ifndef _SYS_SYSENT_H_
#define _SYS_SYSENT_H_

#include <sys/cdefs.h>
#include <sys/syscall.h>
#include <sys/syscallargs.h>

typedef struct ctx ctx_t;
typedef struct mcontext mcontext_t;
typedef struct proc proc_t;

typedef int syscall_t(proc_t *, void *, register_t *);

typedef struct syscall_result {
  register_t retval;
  register_t error;
} syscall_result_t;

typedef struct sysent {
  const char *name; /* syscall name for logging purposes */
  int nargs;        /* number of args passed to syscall */
  syscall_t *call;  /* syscall implementation */
} sysent_t;

extern struct sysent sysent[];

void syscall_handler(int code, ctx_t *ctx, syscall_result_t *result);
void syscall_set_retval(mcontext_t *ctx, syscall_result_t *result, int sig);

#endif /* !_SYS_SYSENT_H_ */
