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
  SIGBUS,
  NSIG = 32
} signo_t;

/* Machine-dependent signal definitions */
typedef int sig_atomic_t;

typedef void (*sighandler_t)(int);

#define SIG_ERR ((sighandler_t)-1)
#define SIG_DFL ((sighandler_t)0)
#define SIG_IGN ((sighandler_t)1)

typedef struct sigaction {
  sighandler_t sa_handler;
  void *sa_restorer;
} sigaction_t;

#ifndef _KERNELSPACE
#include <sys/unistd.h>

int sigaction(int signum, const sigaction_t *act, sigaction_t *oldact);
void sigreturn(void);
int kill(int pid, int sig);
int killpg(int pgrp, int sig);

/* This is necessary to keep newlib happy. */
typedef sighandler_t _sig_func_ptr;

#else /* _KERNELSPACE */

#include <sys/cdefs.h>
#include <sys/bitstring.h>

typedef struct proc proc_t;
typedef struct thread thread_t;
typedef struct exc_frame exc_frame_t;

typedef bitstr_t sigset_t[bitstr_size(NSIG)];

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

#endif /* !_KERNELSPACE */

#endif /* _SYS_SIGNAL_H_ */
