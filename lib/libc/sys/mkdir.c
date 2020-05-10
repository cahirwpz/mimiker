#include <fcntl.h>
#include <sys/stat.h>

int mkdir(const char *path, mode_t mode) {
  return mkdirat(AT_FDCWD, path, mode);
}
