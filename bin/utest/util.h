#pragma once

#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

/* Wait for a single child process with process id `pid` to exit,
 * and check that its exit code matches the expected value. */
void wait_for_child_exit(int pid, int exit_code);

/* Do the necessary setup needed to wait for the signal.
 * Must be called before receiving the signal,
 * ideally at the very beginning of the test procedure. */
void signal_setup(int signo);

/* Wait for the delivery of a signal. */
void wait_for_signal(int signo);

/* Create a new pseudoterminal and return file descriptors to the master and
 * slave side. */
void open_pty(int *master_fd, int *slave_fd);

extern jmp_buf _expect_signal_ctx;

void _expect_signal_setup(int signo, siginfo_t *siginfop);
void _expect_signal_cleanup(void);

#define EXPECT_SIGNAL(signo, siginfop)                                         \
  for (_expect_signal_setup((signo), (siginfop));                              \
       sigsetjmp(_expect_signal_ctx, 1) == 0; exit(1))

#define CLEANUP_SIGNAL() _expect_signal_cleanup()

#define CHECK_SIGSEGV(si, sig_addr, sig_code)                                  \
  assert((si)->si_addr == (sig_addr));                                         \
  assert((si)->si_code == (sig_code))

#define assert_ok(expr) assert((long int)(expr) == 0)
#define assert_fail(expr, err) assert((long int)(expr) == -1 && errno == err)

#define STRING_EQ(str1, str2)                                                  \
  ({                                                                           \
    int __ret = strcmp(str1, str2);                                            \
    assert_ok(__ret);                                                          \
  })

#define STRING_NE(str1, str2)                                                  \
  ({                                                                           \
    int __ret = strcmp((str1), (str2));                                        \
    assert(__ret != 0);                                                        \
  })

#ifndef FD_OFFSET
#define FD_OFFSET 0
#endif

#undef assert_open_ok
#define assert_open_ok(fd, file, mode, flag)                                   \
  n = open(file, flag, mode);                                                  \
  assert(n == fd + FD_OFFSET);

#undef assert_open_fail
#define assert_open_fail(file, mode, flag, err)                                \
  n = open(file, flag, 0);                                                     \
  assert(n < 0);                                                               \
  assert(errno == err);

#undef assert_read_ok
#define assert_read_ok(fd, buf, len)                                           \
  n = read(fd + FD_OFFSET, buf, len);                                          \
  assert(n >= 0);

#undef assert_read_equal
#define assert_read_equal(fd, buf, str)                                        \
  {                                                                            \
    int len = strlen(str);                                                     \
    n = read(fd + FD_OFFSET, buf, len);                                        \
    assert(strncmp(str, buf, len) == 0);                                       \
    assert(n >= 0);                                                            \
  }

#undef assert_read_fail
#define assert_read_fail(fd, buf, len, err)                                    \
  n = read(fd + FD_OFFSET, buf, len);                                          \
  assert(n < 0);                                                               \
  assert(errno == err);

#undef assert_write_ok
#define assert_write_ok(fd, buf, len)                                          \
  n = write(fd + FD_OFFSET, buf, len);                                         \
  assert(n >= 0);

#undef assert_write_fail
#define assert_write_fail(fd, buf, len, err)                                   \
  n = write(fd + FD_OFFSET, buf, len);                                         \
  assert(n < 0);                                                               \
  assert(errno == err);

#undef assert_close_ok
#define assert_close_ok(fd)                                                    \
  n = close(fd + FD_OFFSET);                                                   \
  assert(n == 0);

#undef assert_close_fail
#define assert_close_fail(fd, err)                                             \
  n = close(fd + FD_OFFSET);                                                   \
  assert(n < 0);                                                               \
  assert(errno == err);

#undef assert_lseek_ok
#define assert_lseek_ok(fd, offset, whence)                                    \
  n = lseek(fd + FD_OFFSET, offset, whence);                                   \
  assert(n >= 0);

#undef assert_pipe_ok
#define assert_pipe_ok(fds)                                                    \
  n = pipe(fds);                                                               \
  assert(n == 0);
