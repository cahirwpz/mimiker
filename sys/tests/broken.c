#include <sys/klog.h>
#include <sys/ktest.h>

static int test_broken(void) {

  /* Invalid memory access. */
  volatile char *c = (char *)0x55555555;
  char v = *c;
  (void)v;

  /* Failing assertion */
  assert(0);

  /* Failure exit code */
  return KTEST_FAILURE;
}

/* This test is intentionally broken in many ways. It won't run automatically
   and is only useful for debugging the testing framework. */
KTEST_ADD(broken_test, test_broken, KTEST_FLAG_BROKEN);
