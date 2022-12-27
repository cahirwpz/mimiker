#include <fcntl.h>
#include <unistd.h>

int access(const char *path, int mode) {
  return faccessat(AT_FDCWD, path, mode, 0);
}
