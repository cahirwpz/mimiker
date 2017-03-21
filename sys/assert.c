#include <common.h>
#include <stdc.h>
#include <ktest.h>

void assert_fail(const char *expr, const char *file, unsigned int line) {
  kprintf("Assertion \"%s\" at [%s:%d] failed!", expr, file, line);
  if (ktest_test_running_flag)
    ktest_failure();
  else
    panic("Assertion failed.");
}
