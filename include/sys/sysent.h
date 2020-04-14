#ifndef _SYS_SYSENT_H_
#define _SYS_SYSENT_H_

#include <sys/cdefs.h>
#include <sys/syscall.h>
#include <sys/syscallargs.h>

typedef struct proc proc_t;

typedef int syscall_t(proc_t *, void *, register_t *);

typedef struct sysent {
  int nargs;       /* number of args passed to syscall */
  syscall_t *call; /* syscall implementation */
} sysent_t;

extern struct sysent sysent[];

#endif /* !_SYS_SYSENT_H_ */
