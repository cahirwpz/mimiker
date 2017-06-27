/* Userspace program testing lseek */

#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>

/* ascii file stores consecutive ASCII characters from range 32..127 */
const char *testfile = "/tests/ascii";

#define pos(x) ((x) + 32)
#define end (95)

static int readchar(int fd) {
  char c;
  int n = read(fd, &c, 1);
  if (n == 1)
    return c;
  return -1;
}

static void check_lseek(int fd, off_t offset, int whence, off_t expect) {
  assert(lseek(fd, offset, whence) == expect);
  assert(readchar(fd) == pos(expect));
}

static void check_lseek_set(int fd, off_t offset) {
  check_lseek(fd, offset, SEEK_SET, offset);
}

static void check_lseek_cur(int fd, off_t offset, off_t expect) {
  check_lseek(fd, offset, SEEK_CUR, expect);
}

static void check_lseek_end(int fd, off_t offset, off_t expect) {
  check_lseek(fd, offset, SEEK_END, expect);
}

int test_lseek_basic(void) {
  int fd;

  /* Test SEEK_SET behaviour. */
  fd = open(testfile, 0, O_RDONLY);
  check_lseek_set(fd, 0);
  check_lseek_set(fd, 42);
  check_lseek_set(fd, end - 1);
  close(fd);

  /* Test SEEK_CUR behaviour. */
  fd = open(testfile, 0, O_RDONLY);
  check_lseek_cur(fd, 20, 20);
  check_lseek_cur(fd, -1, 20);
  check_lseek_cur(fd, end - 22, end - 1);
  check_lseek_cur(fd, -end, 0);
  close(fd);

  /* Test SEEK_CUR behaviour. */
  fd = open(testfile, 0, O_RDONLY);
  check_lseek_end(fd, -1, end - 1);
  check_lseek_end(fd, -20, end - 20);
  check_lseek_end(fd, -end, 0);
  close(fd);

  return 0;
}

int test_lseek_errors(void) {
  return 0;
}
