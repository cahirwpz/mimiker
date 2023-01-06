#ifndef __UTEST_H__
#define __UTEST_H__

#include <sys/linker_set.h>
#include <stdnoreturn.h>

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

noreturn void utest_die(const char *file, int line, const char *func,
                        const char *expr);
int utest_spawn(proc_func_t func, void *arg);
void utest_child_exited(int exitcode);

#define assert(e) ((e) ?: utest_die(__FILE__, __LINE__, __func__, #e))

/* Test if system call returned with no error. */
#define syscall_ok(e) (!(e) ?: utest_die(__FILE__, __LINE__, __func__, #e))

/* Test if system call returned with specific error. */
#define syscall_fail(e, err)                                                   \
  (((long)(e) == -1 && errno == (err))                                         \
     ?: utest_die(__FILE__, __LINE__, __func__, #e))

#define string_eq(s1, s2)                                                      \
  (!strcmp((s1), (s2)) ?: utest_die(__FILE__, __LINE__, __func__, "s1 != s2"))

#define string_ne(s1, s2)                                                      \
  (strcmp((s1), (s2)) ?: utest_die(__FILE__, __LINE__, __func__, "s1 == s2"))

#endif /* __UTEST_H__ */
