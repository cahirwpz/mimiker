#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

int getgroups(int gidsetlen, gid_t *gidset) {
  if (gidsetlen < 1) {
    errno = EINVAL;
    return -1;
  }

  gidset[0] = 0;
  return 1;
}
