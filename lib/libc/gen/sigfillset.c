#include <signal.h>

int sigfillset(sigset_t *set) {
  __sigfillset(set);
  return 0;
}
