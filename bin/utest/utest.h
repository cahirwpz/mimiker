#ifndef __UTEST_H__
#define __UTEST_H__

#include <sys/linker_set.h>

typedef int (*test_func_t)(void);

typedef struct test_entry {
  const char *name;
  test_func_t func;
} test_entry_t;

#define TEST_ADD(name)                                                         \
  int test_##name(void);                                                       \
  test_entry_t name##_test = {#name, test_##name};                             \
  SET_ENTRY(tests, name##_test);                                               \
  int test_##name(void)

typedef int (*proc_func_t)(void *);

int utest_spawn(proc_func_t func, void *arg);
void utest_child_exited(int exitcode);

#endif /* __UTEST_H__ */
