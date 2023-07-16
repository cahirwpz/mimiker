#include "utest.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

TEST_ADD(stat, 0) {
  struct stat sb;

  syscall_ok(stat("/tests/ascii", &sb));
  assert(sb.st_size == 95);
  assert(S_ISREG(sb.st_mode));

  syscall_fail(stat("/tests/ascii", NULL), EFAULT);
  syscall_fail(stat("/dont/exist", &sb), ENOENT);
  return 0;
}

TEST_ADD(fstat, 0) {
  struct stat sb;
  int fd;

  fd = open("/tests", 0, O_RDONLY);
  syscall_ok(fstat(fd, &sb));
  assert(S_ISDIR(sb.st_mode));

  syscall_fail(fstat(fd, NULL), EFAULT);
  syscall_fail(fstat(666, &sb), EBADF);
  close(fd);

  return 0;
}
