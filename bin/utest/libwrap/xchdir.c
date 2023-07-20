#include "utest.h"
#include <unistd.h>

void xchdir(const char *path) {
  int result = chdir(path);

  if (result < 0)
    die("chdir: %s", strerror(errno));
}
