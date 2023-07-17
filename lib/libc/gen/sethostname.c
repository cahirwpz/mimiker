#include <errno.h>
#include <unistd.h>

int sethostname(const char *name, size_t namelen) {
  errno = ENOTSUP;
  return -1;
}
