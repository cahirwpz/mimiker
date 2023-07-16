#include "utest.h"
#include <unistd.h>
#include <sys/stat.h>

void xstat(const char *pathname, struct stat *statbuf) {
  int result = stat(pathname, statbuf);

  if (result < 0)
    die("stat: %s", strerror(errno));
}
