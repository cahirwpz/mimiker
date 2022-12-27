#include <sys/stat.h>
#include <fcntl.h>

int chmod(const char *path, mode_t mode) {
  return fchmodat(AT_FDCWD, path, mode, 0);
}
