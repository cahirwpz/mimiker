#include "utest.h"
#include <unistd.h>

void xsymlink(const char *target, const char *linkpath) {
  int result = symlink(target, linkpath);

  if (result < 0)
    die("symlink: %s", strerror(errno));
}
