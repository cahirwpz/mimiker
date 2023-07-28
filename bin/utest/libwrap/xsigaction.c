#include "utest.h"
#include <signal.h>

void xsigaction(int sig, const struct sigaction *restrict act,
                struct sigaction *restrict oact) {
  int result = sigaction(sig, act, oact);

  if (result < 0)
    die("sigaction: %s", strerror(errno));
}
