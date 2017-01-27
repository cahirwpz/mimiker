#ifndef _SYS_KTEST_H_
#define _SYS_KTEST_H_

#include <linker_set.h>
#include <stdc.h>

#define KTEST_NAME_MAX 32

#define KTEST_SUCCESS 0
#define KTEST_FAILURE 1

/* Signifies that a test does not return. */
#define KTEST_FLAG_NORETURN 0x01
/* Signifies that a test irreversibly breaks internal kernel state, and any
   further test done without restarting the kernel will be inconclusive. */
#define KTEST_FLAG_DIRTY 0x02
/* Indicates that a test enters usermode. */
#define KTEST_FLAG_USERMODE 0x04
/* Excludes the test from being run in auto mode. This flag is only useful for
   temporarily marking some tests while debugging the testing framework. */
#define KTEST_FLAG_BROKEN 0x08

typedef struct {
  const char test_name[KTEST_NAME_MAX];
  int (*test_func)();
  uint32_t flags;
} test_entry_t;

void ktest_main(const char *test);

#define KTEST_ADD_FLAGS(name, func, flags)                                     \
  test_entry_t name##_test = {#name, func, flags};                             \
  SET_ENTRY(tests, name##_test);

#define KTEST_ADD(name, func) KTEST_ADD_FLAGS(name, func, 0)

/* These are canonical result messages printed to standard output / UART. A
 * script running the kernel may want to grep for them. */
#define TEST_PASSED_STRING "[TEST PASSED]\n"
#define TEST_FAILED_STRING "[TEST FAILED]\n"

/* This function is called both by run_test, as well as ktest_assert. It
 * displays some troubleshooting info about the failing test. */
void ktest_failure();

/* This assert variant will call ktest_failure when assertion fails, which
   prints out some useful information about the failing test case. */
#define ktest_assert(EXPR)                                                     \
  __extension__({                                                              \
    if (!(EXPR)) {                                                             \
      kprintf("Assertion '" __STRING(EXPR) "' at %s:%d failed!\n", __FILE__,   \
              __LINE__);                                                       \
      ktest_failure();                                                         \
    }                                                                          \
  })

/* This flag is set to 1 when a kernel test is in progress, and 0 otherwise. */
extern int ktest_test_running_flag;

#endif /* !_SYS_KTEST_H_ */
