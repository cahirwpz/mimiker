#ifndef _SYS_SIGNAL_H_
#define _SYS_SIGNAL_H_

#include <sys/sigtypes.h>
#include <sys/siginfo.h>

typedef enum {
  SIGHUP = 1,
  SIGINT = 2,
  SIGQUIT = 3,
  SIGILL = 4,
  SIGTRAP = 5,
  SIGABRT = 6,
  SIGFPE = 8,
  SIGKILL = 9,
  SIGBUS = 10,
  SIGSEGV = 11,
  SIGSYS = 12,
  SIGPIPE = 13,
  SIGALRM = 14,
  SIGTERM = 15,
  SIGSTOP = 17,
  SIGCONT = 19,
  SIGCHLD = 20,
  SIGUSR1 = 30,
  SIGUSR2 = 31,
  NSIG = 32
} signo_t;

/* Machine-dependent signal definitions */
typedef int sig_atomic_t;

typedef void (*sighandler_t)(int);

#define SIG_ERR ((sighandler_t)-1)
#define SIG_DFL ((sighandler_t)0)
#define SIG_IGN ((sighandler_t)1)

typedef struct sigaction {
  union {
    sighandler_t sa_handler;
    void (*sa_sigaction)(int, siginfo_t *, void *);
  };
  sigset_t sa_mask;
  int sa_flags;

  void *sa_restorer;
} sigaction_t;

/* Flags for sigprocmask(): */
#define SIG_BLOCK 1   /* block specified signal set */
#define SIG_UNBLOCK 2 /* unblock specified signal set */
#define SIG_SETMASK 3 /* set specified signal set */

#ifndef _KERNEL
#include <sys/unistd.h>

int sigaction(int signum, const sigaction_t *act, sigaction_t *oldact);
void sigreturn(void);
int kill(int pid, int sig);
int killpg(int pgrp, int sig);

/* This is necessary to keep newlib happy. */
typedef sighandler_t _sig_func_ptr;

#else /* _KERNEL */

#include <sys/cdefs.h>

typedef struct proc proc_t;
typedef struct thread thread_t;
typedef struct exc_frame exc_frame_t;

/*! \brief Signal a process.
 *
 * Marks \a sig signal as pending, unless it's ignored by target process.
 * In most cases signal delivery wakes up the process. Signal handling is
 * usually performed in the context of target process.
 *
 * \sa sig_post
 * \note must be called with p::p_lock held
 */
void sig_kill(proc_t *p, signo_t sig);

/*! \brief Determines which signal should posted to current thread.
 *
 * A signal that meets one of following criteria should be posted:
 *  - has a handler registered with `sigaction`,
 *  - should cause the process to terminate,
 *  - should interrupt the current system call.
 *
 * \sa sig_post
 *
 * \returns signal number which should be posted or 0 if none */
int sig_check(thread_t *td);

/*! \brief Invoke the action triggered by a signal.
 *
 * If the default action for a signal is to terminate the process and
 * corresponding signal handler is not set, the process calls `sig_exit`.
 *
 * \sa sig_exit
 */
void sig_post(signo_t sig);

/*! \brief Terminate the process as the result of posting a signal. */
__noreturn void sig_exit(thread_t *td, signo_t sig);

/*! \brief Delivers a hardware trap related signal to current thread.
 *
 * \note This is machine dependent code! */
void sig_trap(exc_frame_t *frame, signo_t sig);

/*! \brief Prepare user context for entry to signal handler action.
 *
 * \note This is machine dependent code! */
int sig_send(signo_t sig, sigaction_t *sa);

/*! \brief Restore original user context after signal handler was invoked.
 *
 * \note This is machine dependent code! */
int sig_return(void);

/* System calls implementation. */
int do_sigaction(signo_t sig, const sigaction_t *act, sigaction_t *oldact);
int do_sigreturn(void);

#endif /* !_KERNEL */

#endif /* !_SYS_SIGNAL_H_ */
