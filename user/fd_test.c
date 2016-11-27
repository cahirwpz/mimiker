#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>

int main(int argc, char **argv) {
  const char *str = "Hello world from a user program!\n";

  /* Open a /dev/null */
  int fd0 = open("/dev/null", 0, NULL);
  assert(fd0 == 0);
  /* Open an invalid file */
  int fd1 = open("/proc/cpuinfo", 0, NULL);
  assert(fd1 < 0);
  /* Write to /dev/null */
  int error = 0;
  error = write(fd0, str, strlen(str));
  assert(error >= 0);

  return 0;
}
