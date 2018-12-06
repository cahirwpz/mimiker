#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  int scfd = open("/dev/scancode", O_RDONLY, 0);
  while (1) {
    uint8_t buf[100];
    unsigned n = read(scfd, buf, 100);
    for (unsigned i = 0; i < n; i++) {
      printf("Scancode: %x\n", (int)buf[i]);
    }
  }

  return EXIT_SUCCESS;
}
