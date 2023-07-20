#include "utest.h"
#include <unistd.h>

int xgetgroups(int size, gid_t list[]) {
  int result = getgroups(size, list);

  if (result < 0)
    die("getgroups: %s", strerror(errno));

  return result;
}
