#ifndef _SYS_WAIT_H_
#define _SYS_WAIT_H_

/* XXX: We need a sys/types.h for pid_t. */

#define WNOHANG 1

#ifndef _KERNELSPACE

int wait(int *status);
int waitpid(int pid, int *status, int options);

#else /* _KERNELSPACE */

int do_waitpid(pid_t pid, int *status, int options);

#endif

#endif /* _SYS_WAIT_H_ */
