#ifndef _SYS_SIGNAL_H_
#define _SYS_SIGNAL_H_

typedef enum {
  SIGINT = 1,
  SIGILL,
  SIGABRT,
  SIGFPE,
  SIGSEGV,
  SIGKILL,
  SIGTERM,
  SIGCHLD,
  SIGUSR1,
  SIGUSR2,
  NSIG = 32
} signo_t;

typedef void sighandler_t(int);

#define SIG_DFL (sighandler_t *)0x00
#define SIG_IGN (sighandler_t *)0x01

typedef struct sigaction {
  sighandler_t *sa_handler;
  void *sa_restorer;
} sigaction_t;

#ifndef _KERNELSPACE
#include <unistd.h>

int sigaction(int signum, const sigaction_t *act, sigaction_t *oldact);
void sigreturn();
int kill(int tid, int sig);

static inline int raise(int sig) {
  return kill(getpid(), sig);
}

static inline int signal(int sig, sighandler_t handler) {
  sigaction_t sa = {.sa_handler = handler, .sa_restorer = sigreturn};
  return sigaction(sig, &sa, NULL);
}

/* This is necessary to keep newlib happy. */
typedef sighandler_t _sig_func_ptr;

#else /* _KERNELSPACE */

#include <common.h>
#include <bitstring.h>
#include <queue.h>
#include <mutex.h>

typedef struct proc proc_t;

typedef bitstr_t sigset_t[bitstr_size(NSIG)];

/* Sends a signal to a process. */
int sig_send(proc_t *td, signo_t sig);

/* Process signals pending for the thread. If the thread has received a signal
   that should be caught by the user, return the signal number. */
int sig_check(thread_t *td);

/* Process user action triggered by a signal. */
void sig_deliver(int sig);

/* Arrange for signal delivery when the thread returns from exception context.
 * This must be called whenever new signals are posted to a thread. */
void sig_notify(thread_t *td);

int do_kill(pid_t pid, int sig);
int do_sigaction(int sig, const sigaction_t *act, sigaction_t *oldact);
int do_sigreturn();

int platform_sig_deliver(int sig, sigaction_t *sa);
int platform_sig_return();

#endif /* !_KERNELSPACE */

#endif /* _SYS_SIGNAL_H_ */
