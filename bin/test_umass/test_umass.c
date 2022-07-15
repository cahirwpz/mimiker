#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define BLOCK_SIZE 512
#define WRITE_DATA "Do you like fishsticks?"

int main(int argc, char **argv) {
  printf("USB mass storage device test:\n");
  fflush(stdout);

  int fd = open("/dev/umass", O_RDWR, 0);
  uint8_t buf[BLOCK_SIZE];

  memcpy(buf, WRITE_DATA, sizeof(WRITE_DATA));

  while (1) {
    if (write(fd, buf, BLOCK_SIZE) != BLOCK_SIZE)
      return EXIT_FAILURE;

    memset(buf, 0, BLOCK_SIZE);

    if (read(fd, buf, BLOCK_SIZE) != BLOCK_SIZE)
      return EXIT_FAILURE;

    printf("data: \"%s\"\n", buf);
    fflush(stdout);
  }

  return EXIT_SUCCESS;
}
