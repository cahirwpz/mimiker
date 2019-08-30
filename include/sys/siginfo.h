#ifndef _SYS_SIGINFO_H_
#define _SYS_SIGINFO_H_

typedef union siginfo {
  char si_pad[128]; /* Total size; for future expansion */
} siginfo_t;

#endif /* !_SYS_SIGINFO_H_ */
