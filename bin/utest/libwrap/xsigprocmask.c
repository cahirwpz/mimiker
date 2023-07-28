#include "utest.h"
#include <signal.h>

void xsigprocmask(int how, const sigset_t *restrict set,
                  sigset_t *restrict oset) {
  int result = sigprocmask(how, set, oset);

  if (result < 0)
    die("sigprocmask: %s", strerror(errno));
}
