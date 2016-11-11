#ifndef _SYS_SYSENT_H_
#define _SYS_SYSENT_H_
#include <stdint.h>
#include <mips/mips.h>

typedef struct thread thread_t;

#define SYSCALL_ARGS_MAX 4

typedef struct syscall_args {
  uint32_t code;
  reg_t args[SYSCALL_ARGS_MAX];
} syscall_args_t;

typedef int syscall_t(thread_t *, syscall_args_t *, int *error);

typedef struct { syscall_t *call; } sysent_t;

extern sysent_t sysent[];

#define SYS_EXIT 1
#define SYS_OPEN 2
#define SYS_CLOSE 3
#define SYS_READ 4
#define SYS_WRITE 5
#define SYS_LSEEK 6
#define SYS_UNLINK 7
#define SYS_GETPID 8
#define SYS_KILL 9
#define SYS_FSTAT 10

#define SYS_LAST 10

int syscall(int n);

/* Empty syscall handler, for uninmplemented and depracated syscall numbers. */
int sys_nosys(thread_t *, syscall_args_t *, int *error);

#endif /* !_SYS_SYSENT_H_ */
