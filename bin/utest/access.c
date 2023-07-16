#include "utest.h"
#include "util.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

TEST_ADD(access_basic, 0) {
  /* After implementing credentials test should be extended. */
  syscall_ok(access("/bin/mandelbrot", R_OK));
  syscall_ok(access("/bin/mandelbrot", 0));
  syscall_ok(access("/bin/mandelbrot", R_OK | W_OK | X_OK));
  syscall_fail(access("/tests/ascii", X_OK), EACCES);
  syscall_fail(access("/bin/mandelbrot", (R_OK | W_OK | X_OK) + 1), EINVAL);
  syscall_fail(access("/dont/exist", X_OK), ENOENT);
  syscall_fail(access("dont/exist", X_OK), ENOENT);
  return 0;
}
