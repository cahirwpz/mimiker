#include "utest.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#define assert_ok(expr) assert(expr == 0)
#define assert_fail(expr, err) assert(expr == -1 && errno == err)

int test_stat(void) {
  struct stat sb;

  assert_ok(stat("/tests/ascii", &sb));
  assert(sb.st_size == 95);
  assert(S_ISREG(sb.st_mode));

  assert_fail(stat("/tests/ascii", NULL), EFAULT);
  assert_fail(stat("/dont/exist", &sb), ENOENT);
  return 0;
}

int test_fstat(void) {
  struct stat sb;
  int fd;

  fd = open("/tests", 0, O_RDONLY);
  assert_ok(fstat(fd, &sb));
  assert(S_ISDIR(sb.st_mode));

  assert_fail(fstat(fd, NULL), EFAULT);
  assert_fail(fstat(666, &sb), EBADF);
  close(fd);

  return 0;
}
