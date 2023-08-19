#include <termios.h>
#include <errno.h>

int
tcsendbreak(int fd, int len) {
  errno = ENOTSUP;
  return -1;
}
