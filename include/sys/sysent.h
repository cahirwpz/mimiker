#ifndef _sys_sysent_h_
#define _sys_sysent_h_

#include <sys/cdefs.h>
#include <mips/mips.h>
#include <sys/syscall.h>

typedef struct thread thread_t;

#define syscall_args_max 4

typedef struct syscall_args {
  uint32_t code;
  reg_t args[syscall_args_max];
} syscall_args_t;

typedef int syscall_t(thread_t *, syscall_args_t *);

typedef struct {
  syscall_t *call;
} sysent_t;

extern sysent_t sysent[];

#endif /* !_sys_sysent_h_ */
