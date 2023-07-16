#include "utest.h"
#include <unistd.h>

void xrmdir(const char *pathname) {
  int result = rmdir(pathname);

  if (result < 0)
    die("rmdir: %s", strerror(errno));
}
