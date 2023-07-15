#ifndef __UTEST_H__
#define __UTEST_H__

#include <sys/linker_set.h>
#include <stdnoreturn.h>

/*
 * Test definition
 */

typedef int (*test_func_t)(void);

typedef enum test_flags {
  TF_DISABLED = -1, /* test will return success without being executed */
  TF_DEBUG = 1,     /* display debug messages to stderr */
} test_flags_t;

typedef struct test_entry {
  const char *name;
  test_func_t func;
  test_flags_t flags;
} test_entry_t;

#define TEST_ADD(NAME, FLAGS)                                                  \
  int test_##NAME(void);                                                       \
  test_entry_t NAME##_test = {                                                 \
    .name = #NAME, .func = test_##NAME, .flags = FLAGS};                       \
  SET_ENTRY(tests, NAME##_test);                                               \
  int test_##NAME(void)

/*
 * Diagnostic messages
 */

extern int __verbose;

noreturn void __die(const char *file, int line, const char *func,
                    const char *fmt, ...);
void __msg(const char *file, int line, const char *func, const char *fmt, ...);

#define die(...) __die(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define debug(...) __msg(__FILE__, __LINE__, __func__, __VA_ARGS__)

/*
 * Assertion definition and various checks
 */

#define assert(e)                                                              \
  ({                                                                           \
    if (!(e))                                                                  \
      die("assertion '%s' failed!", #e);                                       \
  })

/* Test if system call returned with no error. */
#define syscall_ok(e)                                                          \
  ({                                                                           \
    errno = 0;                                                                 \
    long __r = (long)(e);                                                      \
    if (__r != 0)                                                              \
      die("call '%s' unexpectedly returned %ld (errno = %d)!", #e, __r,        \
          errno);                                                              \
  })

/* Test if system call returned with specific error. */
#define syscall_fail(e, err)                                                   \
  ({                                                                           \
    errno = 0;                                                                 \
    long __r = (long)(e);                                                      \
    if ((__r != -1) || (errno != (err)))                                       \
      die("call '%s' unexpectedly returned %ld (errno = %d)", #e, __r, errno); \
  })

#define string_eq(s1, s2)                                                      \
  ({                                                                           \
    if (strcmp((s1), (s2)))                                                    \
      die("strings were expected to match!");                                  \
  })

#define string_ne(s1, s2)                                                      \
  ({                                                                           \
    if (!strcmp((s1), (s2)))                                                   \
      die("strings were not expected to match!");                              \
  })

/* Miscellanous helper functions */

typedef int (*proc_func_t)(void *);

int utest_spawn(proc_func_t func, void *arg);
void utest_child_exited(int exitcode);

/* libc functions wrappers that automatically call die() on error */

#include <unistd.h>

pid_t xfork(void);

#endif /* __UTEST_H__ */
