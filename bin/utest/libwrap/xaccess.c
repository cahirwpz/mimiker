#include "utest.h"
#include <unistd.h>

void xaccess(const char *pathname, int mode) {
  int result = access(pathname, mode);

  if (result < 0)
    die("access: %s", strerror(errno));
}
