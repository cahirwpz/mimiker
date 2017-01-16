#ifndef _SYS_KTEST_H_
#define _SYS_KTEST_H_

#include <linker_set.h>

#define TEST_NAME_MAX 32

typedef struct {
  const char test_name[TEST_NAME_MAX];
  int (*test_func)();
} test_entry_t;

#define KTEST_ADD(name, func)                                                   \
  test_entry_t name##_test = {#name, func};                                    \
  SET_ENTRY(tests, name##_test);

#endif /* !_SYS_KTEST_H_ */
