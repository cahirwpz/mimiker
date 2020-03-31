#include <fcntl.h>
#include <stdarg.h>

int open(const char *path, int flags, ...) {
  int ret;
  va_list args;
  va_start(args, flags);
  ret = openat(AT_FDCWD, path, flags, args);
  va_end(args);
  return ret;
}
