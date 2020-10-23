#include <errno.h>
#include <unistd.h>

const char *ttyname(int fd) {
  /* TODO(fzdob): to implement */
  errno = ENOTTY;
  return NULL;
}
