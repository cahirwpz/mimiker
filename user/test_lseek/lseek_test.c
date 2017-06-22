/* Userspace program testing lseek */

#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>

static void test_set(const char *dir_path) {
  int fd = open(dir_path, 0, O_RDONLY);
  const int size = 256;
  char buf[size + 1];
  read(fd, buf, size);
  lseek(fd, 0, SEEK_SET);
  char buf2[size + 1];
  read(fd, buf2, size);
  assert(strcmp(buf, buf2) == 0);
  close(fd);
}
static void test_cur(const char *dir_path) {
  int fd = open(dir_path, 0, O_RDONLY);
  const int size = 256;
  char buf[size + 1];
  read(fd, buf, size);

  const int shift = 128;
  lseek(fd, -shift, SEEK_CUR);
  char buf2[size + 1];
  read(fd, buf2, size);
  assert(strcmp(buf + size - shift, buf2) == 0);
  close(fd);
}
static void test_end(const char *dir_path) {
  int fd = open(dir_path, 0, O_RDONLY);
  const int size = 256;
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
}

int main(int argc, char **argv) {
  const char *mypath = "/bin/lseek_test";
  test_set(argc > 1 ? argv[1] : mypath);
  test_cur(argc > 1 ? argv[1] : mypath);
  test_end(argc > 1 ? argv[1] : mypath);
  return 0;
}
