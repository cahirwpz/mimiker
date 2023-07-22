#include "utest.h"
#include <sys/stat.h>

void xmkdir(const char *pathname, mode_t mode) {
  int result = mkdir(pathname, mode);

  if (result < 0)
    die("mkdir: %s", strerror(errno));
}
