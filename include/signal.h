#ifndef _SYS_SIGNAL_H_
#define _SYS_SIGNAL_H_

#include <common.h>
#include <bitstring.h>
#include <queue.h>
#include <mutex.h>
#include <signum.h>

typedef struct proc proc_t;

typedef bitstr_t sigset_t[bitstr_size(SIG_LAST)];

typedef void (*sighandler_t)(int);

#define SIG_DFL (sighandler_t *)0x00
#define SIG_IGN (sighandler_t *)0x01
#define SIG_TERM (sighandler_t *)0x02

typedef struct sigaction {
  sighandler_t *sa_handler;
  void *sa_restorer;
} sigaction_t;

/* Sends a signal to a process. */
int signal(proc_t *td, signo_t sig);

/* Process signals pending for the thread. If the thread has received a signal
   that should be caught by the user, return the signal number. */
int issignal(thread_t *td);

/* Process user action triggered by a signal. */
void postsig(int sig);

/* Arrange for signal delivery when the thread returns from exception context.
 * This must be called whenever new signals are posted to a thread. */
void signotify(thread_t *td);

int do_kill(pid_t pid, int sig);
int do_sigaction(int sig, const sigaction_t *act, sigaction_t *oldact);
int do_sigreturn();

/* Syscall interface */
typedef struct thread thread_t;
typedef struct syscall_args syscall_args_t;
int sys_kill(thread_t *td, syscall_args_t *args);
int sys_sigaction(thread_t *td, syscall_args_t *args);
int sys_sigreturn(thread_t *td, syscall_args_t *args);

#endif /* _SYS_SIGNAL_H_ */
