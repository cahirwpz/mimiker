#ifndef _SYS_SYSENT_H_
#define _SYS_SYSENT_H_

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
#define SYS_SBRK 11
#define SYS_MMAP 12
#define SYS_FORK 13
#define SYS_MOUNT 14
#define SYS_GETDENTS 15
#define SYS_DUP 16
#define SYS_DUP2 17
#define SYS_SIGACTION 18
#define SYS_SIGRETURN 19
#define SYS_WAITPID 20
#define SYS_MKDIR 21
#define SYS_RMDIR 22
#define SYS_ACCESS 23
#define SYS_STAT 24
#define SYS_LAST 25

#ifndef __ASSEMBLER__

#include <stdint.h>
#include <mips/mips.h>

typedef struct thread thread_t;

#define SYSCALL_ARGS_MAX 4

typedef struct syscall_args {
  uint32_t code;
  reg_t args[SYSCALL_ARGS_MAX];
} syscall_args_t;

typedef int syscall_t(thread_t *, syscall_args_t *);

typedef struct { syscall_t *call; } sysent_t;

extern sysent_t sysent[];

int syscall(int n);

#endif /* __ASSEMBLER__ */

#endif /* !_SYS_SYSENT_H_ */
