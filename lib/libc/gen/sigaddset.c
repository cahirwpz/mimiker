#include <errno.h>
#include <signal.h>

int sigaddset(sigset_t *set, int signo) {
  if (signo <= 0 || signo >= NSIG) {
    errno = EINVAL;
    return -1;
  }
  __sigaddset(set, signo);
  return (0);
}
