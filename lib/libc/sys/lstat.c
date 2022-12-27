#include <sys/stat.h>
#include <fcntl.h>

int lstat(const char *pathname, struct stat *statbuf) {
  return fstatat(AT_FDCWD, pathname, statbuf, AT_SYMLINK_NOFOLLOW);
}
