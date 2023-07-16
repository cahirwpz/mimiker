/* Userspace program testing lseek */

#include "utest.h"
#include "util.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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

TEST_ADD(lseek_basic, 0) {
  int fd;

  fd = xopen(testfile, 0, O_RDONLY);
  check_lseek(fd, 0, SEEK_SET, 0);
  check_lseek(fd, 42, SEEK_SET, 42);
  check_lseek(fd, end - 1, SEEK_SET, end - 1);
  xclose(fd);

  fd = xopen(testfile, 0, O_RDONLY);
  check_lseek(fd, -1, SEEK_END, end - 1);
  check_lseek(fd, -20, SEEK_END, end - 20);
  check_lseek(fd, -end, SEEK_END, 0);
  xclose(fd);

  fd = xopen(testfile, 0, O_RDONLY);
  check_lseek(fd, 20, SEEK_CUR, 20);
  check_lseek(fd, -1, SEEK_CUR, 20);
  check_lseek(fd, end - 22, SEEK_CUR, end - 1);
  check_lseek(fd, -end, SEEK_CUR, 0);
  xclose(fd);

  return 0;
}

TEST_ADD(lseek_errors, 0) {
  int fd;

  fd = xopen(testfile, 0, O_RDONLY);
  assert(lseek(fd, end, SEEK_SET) == end);
  syscall_fail(lseek(fd, end + 1, SEEK_SET), EINVAL);
  syscall_fail(lseek(fd, -1, SEEK_SET), EINVAL);
  xclose(fd);

  fd = xopen(testfile, 0, O_RDONLY);
  assert(lseek(fd, -end, SEEK_END) == 0);
  syscall_fail(lseek(fd, -end - 1, SEEK_END), EINVAL);
  syscall_fail(lseek(fd, 1, SEEK_END), EINVAL);
  xclose(fd);

  fd = xopen(testfile, 0, O_RDONLY);
  assert(lseek(fd, 47, SEEK_CUR) == 47);
  syscall_fail(lseek(fd, -48, SEEK_CUR), EINVAL);
  syscall_fail(lseek(fd, 49, SEEK_CUR), EINVAL);
  xclose(fd);

  /* Now let's check some weird cases. */
  syscall_fail(lseek(666, 10, SEEK_CUR), EBADF);

  fd = xopen("/dev/cons", 0, O_RDONLY);
  syscall_fail(lseek(fd, 10, SEEK_CUR), ESPIPE);
  xclose(fd);

  return 0;
}
