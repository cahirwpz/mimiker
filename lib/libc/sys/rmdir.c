#include <fcntl.h>
#include <unistd.h>

int rmdir(const char *path) {
  return unlinkat(AT_FDCWD, path, AT_REMOVEDIR);
}
