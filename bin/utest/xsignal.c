#include "utest.h"
#include <string.h>
#include <errno.h>
#include <signal.h>

typedef void (*sig_t)(int);

extern sig_t __real_signal(int sig, sig_t func);

sig_t __wrap_signal(int sig, sig_t func) {
  sig_t result = __real_signal(sig, func);

  if (result == SIG_ERR)
    die("signal: %s", strerror(errno));

  return result;
}
