#include <errno.h>
#include <signal.h>

sig_t signal(int sig, sig_t handler) {
  sigaction_t act, oldact;

  if (handler == SIG_ERR || sig < 1 || sig >= NSIG) {
    errno = EINVAL;
    return SIG_ERR;
  }

  act.sa_handler = handler;
  if (sigaction(sig, &act, &oldact) != 0)
    return SIG_ERR;

  return oldact.sa_handler;
}
