#include "utest.h"
#include <unistd.h>

void xpipe(int pipefd[2]) {
  int result = pipe(pipefd);

  if (result < 0)
    die("pipe: %s", strerror(errno));
}
