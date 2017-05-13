#ifndef _SYS_SIGNUM_H_
#define _SYS_SIGNUM_H_

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

  SIG_LAST
} signo_t;

#endif /* _SYS_SIGNUM_H_ */
