#include <fcntl.h>
#include <stdarg.h>

int open(const char *path, int flags, ...) {
  mode_t mode = 0;
  if (flags & O_CREAT) {
    va_list args;
    va_start(args, flags);
    mode = va_arg(args, mode_t);
    va_end(args);
  }
  return openat(AT_FDCWD, path, flags, mode);
}
