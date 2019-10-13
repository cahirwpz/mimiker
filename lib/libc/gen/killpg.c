#include <signal.h>
#include <errno.h>

int killpg(int pgrp, int sig) {
  if (pgrp == 1 || pgrp < 0) {
    errno = ESRCH;
    return -1;
  }
  return kill(-pgrp, sig);
}
