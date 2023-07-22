#pragma once

#include <sys/linker_set.h>
#include <sys/types.h>
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

#include <errno.h>
#include <string.h>

typedef void (*sig_t)(int);
struct stat;
struct timezone;
struct timeval;

void xaccess(const char *pathname, int mode);
void xchdir(const char *path);
void xchmod(const char *pathname, mode_t mode);
void xclose(int fd);
pid_t xfork(void);
void xfstat(int fd, struct stat *statbuf);
int xgetgroups(int size, gid_t *list);
void xgetresuid(uid_t *ruid, uid_t *euid, uid_t *suid);
void xgetresgid(gid_t *rgid, gid_t *egid, gid_t *sgid);
void xgettimeofday(struct timeval *tv, struct timezone *tz);
void xkill(int pid, int sig);
void xkillpg(pid_t pgrp, int sig);
void xlchmod(const char *path, mode_t mode);
void xlink(const char *oldpath, const char *newpath);
void xlstat(const char *pathname, struct stat *statbuf);
void xmkdir(const char *pathname, mode_t mode);
void *xmmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
void xmunmap(void *addr, size_t length);
void xmprotect(void *addr, size_t len, int prot);
int xopen(const char *path, int flags, ...);
void xpipe(int pipefd[2]);
void xrmdir(const char *pathname);
sig_t xsignal(int sig, sig_t func);
void xsetgroups(int size, gid_t *list);
void xsetresuid(uid_t ruid, uid_t euid, uid_t suid);
void xsetresgid(gid_t rgid, gid_t egid, gid_t sgid);
void xstat(const char *pathname, struct stat *statbuf);
void xsymlink(const char *target, const char *linkpath);
void xunlink(const char *pathname);
pid_t xwaitpid(pid_t wpid, int *status, int options);
