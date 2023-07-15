#include "utest.h"
#include <string.h>
#include <errno.h>

extern pid_t __real_fork(void);

pid_t __wrap_fork(void) {
  pid_t result = __real_fork();

  if (result < 0)
    die("fork: %s", strerror(errno));

  return result;
}
