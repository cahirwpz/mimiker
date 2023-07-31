#pragma once

#include <sys/linker_set.h>
#include <sys/types.h>
#include <limits.h>
#include <stdnoreturn.h>

/*
 * Test definition
 */

typedef int (*test_func_t)(void);

typedef enum test_flags {
  TF_DISABLED = INT32_MIN, /* test will return success without being executed */
  TF_DEBUG = 1,            /* display debug messages to stderr */
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
extern const char *testname;

noreturn void __die(const char *file, int line, const char *fmt, ...);
void __msg(const char *file, int line, const char *fmt, ...);

#define die(...) __die(__FILE__, __LINE__, __VA_ARGS__)
#define debug(...) __msg(__FILE__, __LINE__, __VA_ARGS__)

/*
 * Assertion definition and various checks
 */

#define assert(e)                                                              \
  ({                                                                           \
    if (!(e))                                                                  \
      die("assertion '%s' failed!", #e);                                       \
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

/*
 * Miscellanous helper functions
 */

typedef int (*proc_func_t)(void *);

pid_t spawn(proc_func_t func, void *arg);

/* waitpid wrapper that checks if child has exited with given exit code */
pid_t wait_child_exited(pid_t pid, int exitcode);
#define wait_child_finished(pid) wait_child_exited(pid, 0)

/* waitpid wrapper that checks if child has been signaled with given signal */
pid_t wait_child_terminated(pid_t pid, int signo);

void wait_child_stopped(pid_t pid);
void wait_child_continued(pid_t pid);

/*
 * libc function wrappers that call die(...) on error
 */

#include <string.h>
#include <errno.h>

#define NOFAIL_NR(_CALL, ...)                                                  \
  ({                                                                           \
    int _res##__LINE__ = _CALL(__VA_ARGS__);                                   \
    if (_res##__LINE__ == -1)                                                  \
      __die(__FILE__, __LINE__, #_CALL ": %s", strerror(errno));               \
  })

#define NOFAIL(_CALL, _TYPE, ...)                                              \
  ({                                                                           \
    _TYPE _res##__LINE__ = _CALL(__VA_ARGS__);                                 \
    if (_res##__LINE__ == (_TYPE)-1)                                           \
      __die(__FILE__, __LINE__, #_CALL ": %s", strerror(errno));               \
    _res##__LINE__;                                                            \
  })

#define xaccess(...) NOFAIL_NR(access, __VA_ARGS__)
#define xchdir(...) NOFAIL_NR(chdir, __VA_ARGS__)
#define xchmod(...) NOFAIL_NR(chmod, __VA_ARGS__)
#define xclose(...) NOFAIL_NR(close, __VA_ARGS__)
#define xfork(...) NOFAIL(fork, pid_t, __VA_ARGS__)
#define xfstat(...) NOFAIL_NR(fstat, __VA_ARGS__)
#define xgetgroups(...) NOFAIL(getgroups, int, __VA_ARGS__)
#define xgetresuid(...) NOFAIL_NR(getresuid, __VA_ARGS__)
#define xgetresgid(...) NOFAIL_NR(getresgid, __VA_ARGS__)
#define xgettimeofday(...) NOFAIL_NR(gettimeofday, __VA_ARGS__)
#define xkill(...) NOFAIL_NR(kill, __VA_ARGS__)
#define xkillpg(...) NOFAIL_NR(killpg, __VA_ARGS__)
#define xlchmod(...) NOFAIL_NR(lchmod, __VA_ARGS__)
#define xlstat(...) NOFAIL_NR(lstat, __VA_ARGS__)
#define xlink(...) NOFAIL_NR(link, __VA_ARGS__)
#define xmkdir(...) NOFAIL_NR(mkdir, __VA_ARGS__)
#define xmmap(...) NOFAIL(mmap, void *, __VA_ARGS__)
#define xmprotect(...) NOFAIL_NR(mprotect, __VA_ARGS__)
#define xmunmap(...) NOFAIL_NR(munmap, __VA_ARGS__)
#define xopen(...) NOFAIL(open, int, __VA_ARGS__)
#define xpipe(...) NOFAIL_NR(pipe, __VA_ARGS__)
#define xread(...) NOFAIL(read, ssize_t, __VA_ARGS__)
#define xrmdir(...) NOFAIL_NR(rmdir, __VA_ARGS__)
#define xsetgroups(...) NOFAIL_NR(setgroups, __VA_ARGS__)
#define xsetresuid(...) NOFAIL_NR(setresuid, __VA_ARGS__)
#define xsetresgid(...) NOFAIL_NR(setresgid, __VA_ARGS__)
#define xsigaction(...) NOFAIL_NR(sigaction, __VA_ARGS__)
#define xsignal(...) NOFAIL(signal, sig_t, __VA_ARGS__)
#define xsigprocmask(...) NOFAIL_NR(sigprocmask, __VA_ARGS__)
#define xstat(...) NOFAIL_NR(stat, __VA_ARGS__)
#define xsymlink(...) NOFAIL_NR(symlink, __VA_ARGS__)
#define xunlink(...) NOFAIL_NR(unlink, __VA_ARGS__)
#define xwaitpid(...) NOFAIL(waitpid, pid_t, __VA_ARGS__)
#define xwrite(...) NOFAIL(write, ssize_t, __VA_ARGS__)
