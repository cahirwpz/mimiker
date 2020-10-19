#include <unistd.h>

const char *ttyname(int fd) {
  static char name[] = "ttyname not implemented";
  /* TODO(fzdob): to implement */
  return name;
}
