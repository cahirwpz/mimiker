#include "utest.h"
#include <signal.h>

void xkill(pid_t pid, int sig) {
  int result = kill(pid, sig);

  if (result < 0)
    die("kill: %s", strerror(errno));
}
