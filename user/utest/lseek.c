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

int test_lseek_basic(void) {
  int fd;

  fd = open(testfile, 0, O_RDONLY);
  check_lseek(fd, 0, SEEK_SET, 0);
  check_lseek(fd, 42, SEEK_SET, 42);
  check_lseek(fd, end - 1, SEEK_SET, end - 1);
  close(fd);

  fd = open(testfile, 0, O_RDONLY);
  check_lseek(fd, -1, SEEK_END, end - 1);
  check_lseek(fd, -20, SEEK_END, end - 20);
  check_lseek(fd, -end, SEEK_END, 0);
  close(fd);

  fd = open(testfile, 0, O_RDONLY);
  check_lseek(fd, 20, SEEK_CUR, 20);
  check_lseek(fd, -1, SEEK_CUR, 20);
  check_lseek(fd, end - 22, SEEK_CUR, end - 1);
  check_lseek(fd, -end, SEEK_CUR, 0);
  close(fd);

  return 0;
}

#define assert_fail(expr, err) assert(expr == -1 && errno == err)

int test_lseek_errors(void) {
  int fd;

  fd = open(testfile, 0, O_RDONLY);
  assert(lseek(fd, end, SEEK_SET) == end);
  assert_fail(lseek(fd, end + 1, SEEK_SET), EINVAL);
  assert_fail(lseek(fd, -1, SEEK_SET), EINVAL);
  close(fd);

  fd = open(testfile, 0, O_RDONLY);
  assert(lseek(fd, -end, SEEK_END) == 0);
  assert_fail(lseek(fd, -end - 1, SEEK_END), EINVAL);
  assert_fail(lseek(fd, 1, SEEK_END), EINVAL);
  close(fd);

  fd = open(testfile, 0, O_RDONLY);
  assert(lseek(fd, 47, SEEK_CUR) == 47);
  assert_fail(lseek(fd, -48, SEEK_CUR), EINVAL);
  assert_fail(lseek(fd, 49, SEEK_CUR), EINVAL);
  close(fd);

  /* Now let's check some weird cases. */
  assert_fail(lseek(666, 10, SEEK_CUR), EBADF);

  fd = open("/dev/cons", 0, O_RDONLY);
  assert_fail(lseek(fd, 10, SEEK_CUR), ESPIPE);
  close(fd);

  return 0;
}
