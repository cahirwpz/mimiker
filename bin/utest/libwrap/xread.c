#include "utest.h"
#include <unistd.h>

size_t xread(int fd, void *buf, size_t nbytes) {
  ssize_t result = read(fd, buf, nbytes);

  if (result < 0)
    die("read: %s", strerror(errno));

  return result;
}
