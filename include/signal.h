#ifndef _SYS_SIGNAL_H_
#define _SYS_SIGNAL_H_

#include <common.h>
#include <bitstring.h>
#include <queue.h>
#include <mutex.h>

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

typedef bitstr_t sigset_t[bitstr_size(SIG_LAST)];

typedef void (*sighandler_t)(int);

#define SIG_DFL 0x00
#define SIG_IGN 0x01

typedef struct sigaction {
  sighandler_t *sa_handler;
  /* TODO: Flags, sa_sigaction, sigset mask */
} sigaction_t;

typedef struct siginfo {
  signo_t si_signo;
  /* TODO: Sender info, fault address & details, etc. */

  TAILQ_ENTRY(siginfo) si_entry;
} siginfo_t;

typedef TAILQ_HEAD(siginfoqueue, siginfo) siginfoqueue_t;

typedef struct sighand {
  sigaction_t sh_actions[SIG_LAST];

  unsigned sh_refcount;
  mtx_t sh_mtx;
} sighand_t;

sighand_t *sighand_new();
sighand_t *sighand_copy();

void sighand_ref(sighand_t *);
void sighand_unref(sighand_t *);

void sighand_set(signo_t, sigaction_t);

int do_kill(tid_t tid, signo_t sig);
int do_sigaction(signo_t sig, const sigaction_t *act, sigaction_t *oldact);

#endif /* _SYS_SIGNAL_H_ */
