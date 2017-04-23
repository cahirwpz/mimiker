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

#define SIG_DFL (sighandler_t *)0x00
#define SIG_IGN (sighandler_t *)0x01
#define SIG_TERM (sighandler_t *)0x02

typedef struct sigaction {
  sighandler_t *sa_handler;
  /* TODO: Flags, sa_sigaction, sigset mask */
  int flags;
} sigaction_t;

typedef struct sighand {
  sigaction_t sh_actions[SIG_LAST];

  unsigned sh_refcount;
  mtx_t sh_mtx;
} sighand_t;

void sighand_init();

sighand_t *sighand_new();
sighand_t *sighand_copy(sighand_t *sh);

void sighand_ref(sighand_t *);
void sighand_unref(sighand_t *);

int do_kill(tid_t tid, int sig);
int do_sigaction(int sig, const sigaction_t *act, sigaction_t *oldact);

int signal(thread_t *td, signo_t sig);

/* Process signals pending for the thread. If the thread has received a signal
   that should be caught by the user, return the signal number. */
int issignal(thread_t *td);

/* Process user action triggered by a signal. */
void postsig(int sig);

#endif /* _SYS_SIGNAL_H_ */
