#include <errno.h>
#include <signal.h>

int sigdelset(sigset_t *set, int signo) {
  if (signo <= 0 || signo >= NSIG) {
    errno = EINVAL;
    return -1;
  }
  __sigdelset(set, signo);
  return (0);
}
