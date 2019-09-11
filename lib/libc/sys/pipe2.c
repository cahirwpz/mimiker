#include <unistd.h>
#include <fcntl.h>

int pipe2(int fildes[2], int flags) {
  return pipe(fildes);
}
