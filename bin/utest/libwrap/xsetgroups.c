#include "utest.h"
#include <unistd.h>

void xsetgroups(int size, gid_t *list) {
  int result = setgroups(size, list);

  if (result < 0)
    die("setgroups: %s", strerror(errno));
}
