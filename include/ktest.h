#ifndef _SYS_KTEST_H_
#define _SYS_KTEST_H_

#include <linker_set.h>

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

typedef struct {
  const char test_name[KTEST_NAME_MAX];
  int (*test_func)();
  uint32_t flags;
} test_entry_t;

#define KTEST_ADD_FLAGS(name, func, flags)                                     \
  test_entry_t name##_test = {#name, func, flags};                             \
  SET_ENTRY(tests, name##_test);

#define KTEST_ADD(name, func) KTEST_ADD_FLAGS(name, func, 0)

/* These are canonical result messages printed to standard output / UART. A
 * script running the kernel may want to grep for them. */
#define TEST_PASSED_STRING "[TEST PASSED]\n"
#define TEST_FAILED_STRING "[TEST FAILED]\n"

#endif /* !_SYS_KTEST_H_ */
