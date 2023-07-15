#include "utest.h"
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>

int __wrap_kill(pid_t pid, int sig) {
  int result = __real_kill(pid, sig);

  if (result < 0)
    die("kill: %s", strerror(errno));

  return 0;
}
