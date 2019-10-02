#ifndef _SYS_SYSENT_H_
#define _SYS_SYSENT_H_

#include <sys/cdefs.h>
#include <sys/syscall.h>
#include <sys/syscallargs.h>

typedef struct proc proc_t;

typedef int syscall_t(proc_t *, void *, register_t *);

struct sysent {
  syscall_t *call;
};

extern struct sysent sysent[];

#endif /* !_SYS_SYSENT_H_ */
