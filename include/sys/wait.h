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
*/

#define WTERMSIG(status) ((status)&0x7f)
#define WEXITSTATUS(status) (((status)&0xff00) >> 8)
#define WSTOPSIG(status) (((status)&0xff00) >> 8)
#define WIFEXITED(status) (((status)&0x7f) == 0)
#define WIFSTOPPED(status) (((status)&0xff) == 0x7f)
#define WIFSIGNALED(status) (((status)&0x7f) > 0 && ((status)&0x7f) < 0x7f)

#ifndef _KERNEL

int waitpid(pid_t pid, int *status, int options);
#define wait(statusptr) waitpid(-1, statusptr, 0)

#else /* _KERNEL */

int do_waitpid(pid_t pid, int *status, int options, pid_t *childp);

/* Macros for building status values. */
#define MAKE_STATUS_EXIT(exitcode) (((exitcode)&0xff) << 8)
#define MAKE_STATUS_SIG_TERM(signo) ((signo)&0xff)
#define MAKE_STATUS_SIG_STOP(signo) ((((signo)&0xff) << 8) | 0x7f)

#endif /* !_KERNEL */

#endif /* !_SYS_WAIT_H_ */
