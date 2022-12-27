#include <unistd.h>

int pipe(int fildes[2]) {
  return pipe2(fildes, 0);
}
