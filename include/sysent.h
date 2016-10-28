#ifndef _SYS_SYSENT_H_
#define _SYS_SYSENT_H_

typedef struct thread thread_t;

typedef int syscall_t(thread_t *);

typedef struct {
  syscall_t *call;
} sysent_t;

extern sysent_t sysent[];

#define SYS_LAST 0

int syscall(unsigned num);

#endif /* !_SYS_SYSENT_H_ */
