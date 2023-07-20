#include "utest.h"
#include <unistd.h>

void xunlink(const char *pathname) {
  int result = unlink(pathname);

  if (result < 0)
    die("unlink: %s", strerror(errno));
}
