#include <ktest.h>

static int test_broken() {

  ktest_assert(0);

  return KTEST_FAILURE;
}

/* This test is intentionally broken in many ways. It won't run automatically
   and is only useful for debugging the testing framework. */
KTEST_ADD_FLAGS(broken_test, test_broken, KTEST_FLAG_BROKEN);
