#include "utest.h"
#include <signal.h>

sig_t xsignal(int sig, sig_t func) {
  sig_t result = signal(sig, func);

  if (result == SIG_ERR)
    die("signal: %s", strerror(errno));

  return result;
}
