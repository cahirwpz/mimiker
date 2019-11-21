#include <dirent.h>

int getdents(int fd, char *buf, size_t nbytes) {
  return getdirentries(fd, buf, nbytes, NULL);
}
