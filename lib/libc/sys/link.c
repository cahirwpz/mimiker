#include <unistd.h>
#include <fcntl.h>

int link(const char *name1, const char *name2) {
  return linkat(AT_FDCWD, name1, AT_FDCWD, name2, AT_SYMLINK_FOLLOW);
}
