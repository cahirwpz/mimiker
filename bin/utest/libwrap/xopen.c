#include "utest.h"
#include <fcntl.h>
#include <stdarg.h>

int xopen(const char *path, int flags, ...) {
  mode_t mode = 0;
  if (flags & O_CREAT) {
    va_list args;
    va_start(args, flags);
    mode = va_arg(args, mode_t);
    va_end(args);
  }

  int result = open(path, flags, mode);

  if (result < 0)
    die("open: %s", strerror(errno));

  return result;
}
