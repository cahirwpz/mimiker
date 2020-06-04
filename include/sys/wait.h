#ifndef _SYS_WAIT_H_
#define _SYS_WAIT_H_

#include <sys/types.h>

#define WNOHANG 0x00000001    /* don't hang in wait */
#define WSTOPPED 0x00000002   /* include stopped/untraceed children */
#define WUNTRACED WSTOPPED    /* the original name for WSTOPPED */
#define WCONTINUED 0x00000010 /* include continued processes */
#define WEXITED 0x00000020    /* Wait for exited processes. */

/*
  A process exit status consists of two bytes:

    INFO CODE

  This follows the convention many *nixes use.

  CODE == 0:
    The process has exited on its own, and INFO contains the exit value.
  CODE == 1..7e:
    The process has exited due to a signal, and CODE contains signal number.
  CODE == 7f:
    The process has stopped due to a signal, and INFO contains signal number.
  CODE == 80:
    The process got core dumped. We don't use that.
  CODE == 81:
    The process has been continued due to a signal.
*/

#define WTERMSIG(status) ((status)&0x7f)
#define WEXITSTATUS(status) (((status)&0xff00) >> 8)
#define WSTOPSIG(status) (((status)&0xff00) >> 8)
#define WIFEXITED(status) (((status)&0xff) == 0)
#define WIFSTOPPED(status) (((status)&0xff) == 0x7f)
#define WIFSIGNALED(status) (((status)&0xff) > 0 && ((status)&0xff) < 0x7f)
#define WIFCONTINUED(status) ((status) == 0x81)

#ifndef _KERNEL

struct rusage;
struct krusage;

pid_t wait(int *);
pid_t waitpid(pid_t pid, int *status, int options);
pid_t wait4(pid_t pid, int *status, int options, struct rusage *rusage);
pid_t _wait4(pid_t pid, int *status, int options, struct krusage *krusage);

#else /* _KERNEL */

int do_waitpid(pid_t pid, int *status, int options, pid_t *childp);

/* Macros for building status values. */
#define MAKE_STATUS_EXIT(exitcode) (((exitcode)&0xff) << 8)
#define MAKE_STATUS_SIG_TERM(signo) ((signo)&0xff)
#define MAKE_STATUS_SIG_STOP(signo) ((((signo)&0xff) << 8) | 0x7f)
#define MAKE_STATUS_SIG_CONT() (0x81)

#endif /* !_KERNEL */

#endif /* !_SYS_WAIT_H_ */
