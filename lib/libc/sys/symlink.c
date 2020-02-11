#include <fcntl.h>
#include <unistd.h>

int symlink(const char *target, const char *linkpath) {
  return symlinkat(target, AT_FDCWD, linkpath);
}
