#include "utest.h"
#include <unistd.h>
#include <sys/stat.h>

void xfstat(int fd, struct stat *statbuf) {
  int result = fstat(fd, statbuf);

  if (result < 0)
    die("fstat: %s", strerror(errno));
}
