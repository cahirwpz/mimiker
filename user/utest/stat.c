#include "utest.h"

#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#define assert_ok(expr) assert(expr == 0)
#define assert_fail(expr, err) assert(expr == -1 && errno == err)

int test_stat_basic(void) {
  struct stat sb;

  assert_ok(stat("/tests/ascii", &sb));
  assert(sb.st_size == 95);

  assert_fail(stat("/tests/ascii", NULL), EFAULT);
  assert_fail(stat("/dont/exist", &sb), ENOENT);
  return 0;
}
