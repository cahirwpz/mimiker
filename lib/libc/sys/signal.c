#include <errno.h>
#include <string.h>
#include <signal.h>

extern sigset_t __sigintr; /* Shared with siginterrupt() */

sig_t signal(int sig, sig_t handler) {
  sigaction_t act, oldact;

  if (handler == SIG_ERR || sig < 1 || sig >= NSIG) {
    errno = EINVAL;
    return SIG_ERR;
  }

  memset(&act, 0, sizeof(sigaction_t));
  act.sa_handler = handler;
  if (!__sigismember(&__sigintr, sig))
    act.sa_flags |= SA_RESTART;
  if (sigaction(sig, &act, &oldact) != 0)
    return SIG_ERR;

  return oldact.sa_handler;
}
