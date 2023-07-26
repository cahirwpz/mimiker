#include "utest.h"
#include <sys/stat.h>

void xchmod(const char *pathname, mode_t mode) {
  int result = chmod(pathname, mode);

  if (result < 0)
    die("chmod: %s", strerror(errno));
}
