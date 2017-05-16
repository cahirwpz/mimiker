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

#endif /* _SYS_SIGNAL_H_ */
