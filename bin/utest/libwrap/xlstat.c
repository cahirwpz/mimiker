#include "utest.h"
#include <unistd.h>
#include <sys/stat.h>

void xlstat(const char *pathname, struct stat *statbuf) {
  int result = lstat(pathname, statbuf);

  if (result < 0)
    die("lstat: %s", strerror(errno));
}
