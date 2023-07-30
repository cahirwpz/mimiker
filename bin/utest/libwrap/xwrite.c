#include "utest.h"
#include <unistd.h>

size_t xwrite(int fd, const void *buf, size_t nbytes) {
  ssize_t result = write(fd, buf, nbytes);

  if (result < 0)
    die("read: %s", strerror(errno));

  return result;
}
