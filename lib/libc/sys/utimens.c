#include <sys/stat.h>
#include <fcntl.h>

int lutimens(const char *path, const struct timespec times[2]) {
  return utimensat(AT_FDCWD, path, times, 0);
}
