#include <fcntl.h>
#include <unistd.h>

ssize_t readlink(const char *restrict path, char *restrict buf, size_t bufsiz) {
  return readlinkat(AT_FDCWD, path, buf, bufsiz);
}
