#ifndef _SYS_SYSENT_H_
#define _SYS_SYSENT_H_

#include <sys/cdefs.h>
#include <sys/syscall.h>
#include <sys/syscallargs.h>

typedef struct thread thread_t;

typedef int syscall_t(thread_t *, void *);

struct sysent {
  syscall_t *call;
};

extern struct sysent sysent[];

#endif /* !_SYS_SYSENT_H_ */
