#include "utest.h"
#include <unistd.h>

void xlink(const char *oldpath, const char *newpath) {
  int result = link(oldpath, newpath);

  if (result < 0)
    die("link: %s", strerror(errno));
}
