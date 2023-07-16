#include "utest.h"
#include <signal.h>

void xkillpg(pid_t pgrp, int sig) {
  int result = killpg(pgrp, sig);

  if (result < 0)
    die("killpg: %s", strerror(errno));
}
