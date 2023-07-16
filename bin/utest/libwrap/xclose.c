#include "utest.h"
#include <unistd.h>

void xclose(int fd) {
  int result = close(fd);

  if (result < 0)
    die("close: %s", strerror(errno));
}
