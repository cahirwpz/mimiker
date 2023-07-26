#include "utest.h"
#include <sys/wait.h>

pid_t xwaitpid(pid_t wpid, int *status, int options) {
  pid_t result = waitpid(wpid, status, options);

  if (result < 0)
    die("waitpid: %s", strerror(errno));

  return result;
}
