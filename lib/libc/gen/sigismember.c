#include <errno.h>
#include <signal.h>

int sigismember(const sigset_t *set, int signo) {
  if (signo <= 0 || signo >= NSIG) {
    errno = EINVAL;
    return -1;
  }
  return (__sigismember(set, signo));
}
