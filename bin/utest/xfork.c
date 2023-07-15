#include "utest.h"
#include <unistd.h>

pid_t xfork(void) {
  pid_t result = fork();

  if (result < 0)
    die("fork: %s", strerror(errno));

  return result;
}
