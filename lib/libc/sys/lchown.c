#include <unistd.h>
#include <fcntl.h>

int lchown(const char *path, uid_t uid, gid_t gid) {
  return fchownat(AT_FDCWD, path, uid, gid, AT_SYMLINK_NOFOLLOW);
}
