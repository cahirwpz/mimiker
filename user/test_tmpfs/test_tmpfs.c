#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char **argv) {
  int fd = open("/tmp/some_file.c", O_RDWR | O_CREAT | O_TRUNC);
  char *str = "Hello ";
  write(fd, str, strlen(str));
  str = "everyone ";
  write(fd, str, strlen(str));
  str = "from mimiker!";
  write(fd, str, strlen(str) + 1);

  lseek(fd, 0, SEEK_SET);

  char buf[100];
  read(fd, buf, sizeof(buf));

  printf("%s\n", buf);
}
