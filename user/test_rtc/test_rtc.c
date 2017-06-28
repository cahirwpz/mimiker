#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  int fd = open("/dev/rtc", O_RDONLY, 0);
  while (1) {
    int year, month, day, hour, minute, second;
    char buf[100];

    if (read(fd, buf, sizeof(buf)) <= 0)
      return EXIT_FAILURE;

    sscanf(buf, "%d %d %d %d %d %d", &year, &month, &day, &hour, &minute,
           &second);
    printf("Time is %d/%d/%d %02d:%02d:%02d\n", day, month, year, hour, minute,
           second);
  }

  return EXIT_SUCCESS;
}
