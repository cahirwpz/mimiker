/* Userspace program testing lseek */

#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>

const char *testfile = "/tests/ascii";
const int size = 256;

int test_lseek_set(void) {
  int fd = open(testfile, 0, O_RDONLY);
  char buf[size + 1];
  read(fd, buf, size);
  lseek(fd, 0, SEEK_SET);
  char buf2[size + 1];
  read(fd, buf2, size);
  assert(strcmp(buf, buf2) == 0);
  close(fd);
  return 0;
}

int test_lseek_cur(void) {
  int fd = open(testfile, 0, O_RDONLY);
  char buf[size + 1];
  read(fd, buf, size);
  const int shift = 128;
  lseek(fd, -shift, SEEK_CUR);
  char buf2[size + 1];
  read(fd, buf2, size);
  assert(strcmp(buf + size - shift, buf2) == 0);
  close(fd);
  return 0;
}

int test_lseek_end(void) {
  int fd = open(testfile, 0, O_RDONLY);
  char buf[size + 1]; // beginning of file
  int file_size = read(fd, buf, size);

  char tmp[size + 1];
  int read_size;
  int last_size = 0;
  while ((read_size = read(fd, tmp, size))) // reads until end of file
  {
    last_size = file_size;
    file_size += read_size;
  }
  lseek(fd, -file_size, SEEK_END);
  char buf2[size + 1];
  read(fd, buf2, size);
  assert(strcmp(buf, buf2) == 0); // checks if beginning matches

  lseek(fd, -(file_size - last_size), SEEK_END);
  read(fd, buf2, size);
  assert(strcmp(tmp, buf2) == 0); // checks if end maches
  close(fd);
  return 0;
}
