#include <unistd.h>

const char *ttyname(int fd) {
  static char name[] = "/dev/tty";
  /* TODO(fzdob): to implement */
  return name;
}
