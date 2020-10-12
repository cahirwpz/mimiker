#include "utest.h"

#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>

static const char *str = "Hello world from a user program!\n";
static char buf[100];
static int n;

/* Shift used fds by 3 so std{in,out,err} are not affected. */
#define FD_OFFSET 3
#include "utest_fd.h"

/* Just the basic, correct operations on a single /dev/null */
int test_fd_devnull(void) {
  assert_open_ok(0, "/dev/null", 0, O_RDWR);
  assert_read_ok(0, buf, 100);
  assert_write_ok(0, str, strlen(str));
  assert_close_ok(0);
  return 0;
}

/* Opens and closes multiple descriptors, checks if descriptor numbers are
   correctly reused */
int test_fd_multidesc(void) {
  assert_open_ok(0, "/dev/null", 0, O_RDWR);
  assert_open_ok(1, "/dev/null", 0, O_RDWR);
  assert_open_ok(2, "/dev/null", 0, O_RDWR);
  assert_close_ok(1);
  assert_open_ok(1, "/dev/null", 0, O_RDWR);
  assert_close_ok(0);
  assert_close_ok(2);
  assert_open_ok(0, "/dev/null", 0, O_RDWR);
  assert_open_ok(2, "/dev/null", 0, O_RDWR);
  assert_close_ok(1);
  assert_close_ok(0);
  assert_open_ok(0, "/dev/null", 0, O_RDWR);
  assert_open_ok(1, "/dev/null", 0, O_RDWR);
  assert_close_ok(1);
  assert_close_ok(2);
  assert_close_ok(0);
  return 0;
}

/* Tests whether READ/WRITE flags are checked correctly */
int test_fd_readwrite(void) {
  assert_open_ok(0, "/dev/null", 0, O_RDONLY);
  assert_open_ok(1, "/dev/null", 0, O_WRONLY);
  assert_open_ok(2, "/dev/null", 0, O_RDWR);
  /* Test reading, on fd1 it should fail */
  assert_read_ok(0, buf, 100);
  assert_read_fail(1, buf, 100, EBADF);
  assert_read_ok(2, buf, 100);
  /* Test writing, should fail only on fd0 */
  assert_write_fail(0, str, strlen(str), EBADF);
  assert_write_ok(1, str, strlen(str));
  assert_write_ok(2, str, strlen(str));
  /* Close all files */
  assert_close_ok(0);
  assert_close_ok(1);
  assert_close_ok(2);
  return 0;
}

int test_fd_read(void) {
  /* Read all at once */
  const char *contents =
    "This is the content of file \"fd_test_file\" in directory \"/tests\"!";
  assert_open_ok(0, "/tests/fd_test_file", 0, O_RDONLY);
  assert_read_equal(0, buf, contents);
  assert_close_ok(0);

  /* Read in parts */
  assert_open_ok(0, "/tests/fd_test_file", 0, O_RDONLY);
  assert_read_equal(0, buf, "This is the ");
  assert_read_equal(0, buf, "content of file ");
  assert_read_equal(0, buf, "\"fd_test_file\" in directory ");
  assert_read_equal(0, buf, "\"/tests\"!");
  assert_close_ok(0);

  /* Read in parts, using lseek aswell */
  assert_open_ok(0, "/tests/fd_test_file", 0, O_RDONLY);
  assert_lseek_ok(0, strlen("This is the "), SEEK_SET);
  assert_read_equal(0, buf, "content of file ");
  assert_lseek_ok(0, strlen("This is the "), SEEK_SET);
  assert_read_equal(0, buf, "content of file ");
  assert_read_equal(0, buf, "\"fd_test_file\" in directory ");
  assert_read_equal(0, buf, "\"/tests\"!");
  assert_close_ok(0);
  return 0;
}

/* Try passing invalid pointers as arguments to open,read,write. */
int test_fd_copy(void) {
  /* /dev/null does not copy any data, so passing an invalid pointer will not
   * cause any errors - thus we use /dev/zero for this test. However, /dev/zero
   * might also skip copying data, and in that case this test would also fail -
   * but we chose to implement a /dev/zero that copies the provided data into a
   * junk page. */
  assert_open_ok(0, "/dev/zero", 0, O_RDWR);
  /* Kernel space */
  char *naughty_ptr1 = (char *)0x80001000;
  /* User space, hopefully not mapped */
  char *naughty_ptr2 = (char *)0x00001000;
  assert_write_fail(0, naughty_ptr1, 200, EFAULT);
  assert_write_fail(0, naughty_ptr2, 200, EFAULT);
  assert_read_fail(0, naughty_ptr1, 200, EFAULT);
  assert_read_fail(0, naughty_ptr2, 200, EFAULT);
  /* Also, try opening a file using a bad pointer */
  assert_open_fail(naughty_ptr1, 0, O_RDWR, EFAULT);
  assert_open_fail(naughty_ptr2, 0, O_RDWR, EFAULT);
  /* Clean up */
  assert_close_ok(0);
  return 0;
}

/* Tries accessing some invalid descriptor numbers */
int test_fd_bad_desc(void) {
  assert_write_fail(0, buf, 100, EBADF);
  assert_write_fail(42, buf, 100, EBADF);
  assert_write_fail(-10, buf, 100, EBADF);
  assert_read_fail(0, buf, 100, EBADF);
  assert_read_fail(42, buf, 100, EBADF);
  assert_read_fail(-10, buf, 100, EBADF);
  assert_close_fail(0, EBADF);
  assert_close_fail(-10, EBADF);
  assert_close_fail(42, EBADF);
  return 0;
}

int test_fd_open_path(void) {
  assert_open_fail("/etc/shadow", 0, O_RDONLY, ENOENT);
  assert_open_fail("123456", 0, O_RDONLY, ENOENT);
  assert_open_fail("", 0, O_RDONLY, ENOENT);

  assert_open_fail("123456", 0, O_RDONLY, ENOENT);

  /* Also try opening a file with a name too long. */
  char too_long[2000];
  memset(too_long, 'c', sizeof(too_long));
  too_long[sizeof(too_long) - 1] = 0;
  too_long[0] = '/';
  assert_open_fail(too_long, 0, O_RDONLY, ENAMETOOLONG);
  return 0;
}

int test_fd_dup(void) {
  int x = open("/tests/dup_test_file", O_RDONLY, 0);
  int y = dup(0);
  dup2(x, 0);
  char word[100];
  scanf("%[^\n]s", word);
  assert(strcmp(word, "Hello, World!") == 0);
  dup2(y, 0);
  return 0;
}

/* Tests below do not use std* file descriptors */
#undef FD_OFFSET
#include "utest_fd.h"

int test_fd_pipe(void) {
  int fd[2];
  assert_pipe_ok(fd);

  pid_t pid = fork();
  assert(pid >= 0);

  if (pid > 0) {
    /* parent */
    assert_close_ok(fd[0]);
    assert_write_ok(fd[1], str, strlen(str));
    assert_close_ok(fd[1]);

    int status;
    waitpid(-1, &status, 0);
  } else {
    /* child */
    assert_close_ok(fd[1]);
    assert_read_equal(fd[0], buf, str);
    assert_close_ok(fd[0]);
    exit(0);
  }

  return 0;
}

int test_fd_all(void) {
  /* Call all fd-related tests one by one to see how they impact the process
   * file descriptor table. */
  test_fd_read();
  test_fd_devnull();
  test_fd_multidesc();
  test_fd_readwrite();
  test_fd_copy();
  test_fd_bad_desc();
  test_fd_open_path();
  test_fd_pipe();
  test_fd_dup();
  return 0;
}
