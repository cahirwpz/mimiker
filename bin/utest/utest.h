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

noreturn void __die(const char *file, int line, const char *func,
                    const char *fmt, ...);
int utest_spawn(proc_func_t func, void *arg);
void utest_child_exited(int exitcode);

#define die(...) __die(__FILE__, __LINE__, __func__, __VA_ARGS__)

#define assert(e)                                                              \
  ({                                                                           \
    if (!(e))                                                                  \
      die("assertion '%s' failed!\n", #e);                                     \
  })

/* Test if system call returned with no error. */
#define syscall_ok(e)                                                          \
  ({                                                                           \
    errno = 0;                                                                 \
    long __r = (long)(e);                                                      \
    if (__r != 0)                                                              \
      die("call '%s' unexpectedly returned %ld (errno = %d)!\n", #e, __r,      \
          errno);                                                              \
  })

/* Test if system call returned with specific error. */
#define syscall_fail(e, err)                                                   \
  ({                                                                           \
    errno = 0;                                                                 \
    long __r = (long)(e);                                                      \
    if ((__r != -1) || (errno != (err)))                                       \
      die("call '%s' unexpectedly returned %ld (errno = %d)\n", #e, __r,       \
          errno);                                                              \
  })

#define string_eq(s1, s2)                                                      \
  (!strcmp((s1), (s2)) ?: die("strings were expected to match!\n"))

#define string_ne(s1, s2)                                                      \
  (strcmp((s1), (s2)) ?: die("strings were not expected to match!\n"))

#endif /* __UTEST_H__ */
