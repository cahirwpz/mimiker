#include "utest.h"
#include <sys/stat.h>

void xlchmod(const char *pathname, mode_t mode) {
  int result = lchmod(pathname, mode);

  if (result < 0)
    die("lchmod: %s", strerror(errno));
}
