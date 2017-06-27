#include "utest.h"

#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>

#define assert_ok(expr) assert(expr == 0)
#define assert_fail(expr, err) assert(expr == -1 && errno == err)

int test_access_basic(void) {
  /* After implementing credentials test should be extended. */
  assert_ok(access("/bin/mandelbrot", R_OK));
  assert_ok(access("/bin/mandelbrot", 0));
  assert_ok(access("/bin/mandelbrot", R_OK | W_OK | X_OK));
  assert_fail(access("/tests/ascii", X_OK), EACCES);
  assert_fail(access("/bin/mandelbrot", (R_OK | W_OK | X_OK) + 1), EINVAL);
  assert_fail(access("/dont/exist", X_OK), ENOENT);
  assert_fail(access("dont/exist", X_OK), ENOENT);
  return 0;
}
