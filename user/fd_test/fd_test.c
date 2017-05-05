#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

const char *str = "Hello world from a user program!\n";
int error = 0;
int n;
int fd0, fd1, fd2;
char buf[100];

/* The number of standard fds */
#define FD_OFFSET 3

#define assert_open_ok(fd, file, mode, flag)                                   \
  n = open(file, flag, 0);                                                     \
  assert(n == fd + FD_OFFSET);

#define assert_open_fail(file, mode, flag, err)                                \
  n = open(file, flag, 0);                                                     \
  assert(n < 0);                                                               \
  assert(errno == err);

#define assert_read_ok(fd, buf, len)                                           \
  n = read(fd + FD_OFFSET, buf, len);                                          \
  assert(n >= 0);

#define assert_read_equal(fd, buf, str)                                        \
  {                                                                            \
    int len = strlen(str);                                                     \
    n = read(fd + FD_OFFSET, buf, len);                                        \
    assert(strncmp(str, buf, len) == 0);                                       \
    assert(n >= 0);                                                            \
  }

#define assert_read_fail(fd, buf, len, err)                                    \
  n = read(fd + FD_OFFSET, buf, len);                                          \
  assert(n < 0);                                                               \
  assert(errno == err);

#define assert_write_ok(fd, buf, len)                                          \
  n = write(fd + FD_OFFSET, buf, len);                                         \
  assert(n >= 0);

#define assert_write_fail(fd, buf, len, err)                                   \
  n = write(fd + FD_OFFSET, buf, len);                                         \
  assert(n < 0);                                                               \
  assert(errno == err);

#define assert_close_ok(fd)                                                    \
  n = close(fd + FD_OFFSET);                                                   \
  assert(n == 0);

#define assert_close_fail(fd, err)                                             \
  n = close(fd + FD_OFFSET);                                                   \
  assert(n < 0);                                                               \
  assert(errno == err);

#define assert_lseek_ok(fd, offset, whence)                                    \
  n = lseek(fd + FD_OFFSET, offset, whence);                                   \
  assert(n >= 0);

/* Just the basic, correct operations on a single /dev/null */
void test_devnull() {
  assert_open_ok(0, "/dev/null", 0, O_RDWR);
  assert_read_ok(0, buf, 100);
  assert_write_ok(0, str, strlen(str));
  assert_close_ok(0);
}

/* Opens and closes multiple descriptors, checks if descriptor numbers are
   correctly reused */
void test_multiple_descriptors() {
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
}

/* Tests whether READ/WRITE flags are checked correctly */
void test_readwrite() {
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
}

void test_read() {
  /* Read all at once */
  const char *contents = "This is the content of file \"fd_test_file\" in directory \"/tests\"!";
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
}

/* Try passing invalid pointers as arguments to open,read,write. */
void test_copy() {
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
}

/* Tries accessing some invalid descriptor numbers */
void test_bad_descrip() {
  assert_write_fail(0, buf, 100, EBADF);
  assert_write_fail(42, buf, 100, EBADF);
  assert_write_fail(-10, buf, 100, EBADF);
  assert_read_fail(0, buf, 100, EBADF);
  assert_read_fail(42, buf, 100, EBADF);
  assert_read_fail(-10, buf, 100, EBADF);
  assert_close_fail(0, EBADF);
  assert_close_fail(-10, EBADF);
  assert_close_fail(42, EBADF);
}

void test_open_path() {
  assert_open_fail("/etc/shadow", 0, O_RDONLY, ENOENT);
  assert_open_fail("123456", 0, O_RDONLY, ENOENT);
  assert_open_fail("", 0, O_RDONLY, ENOENT);

  assert_open_fail("123456", 0, O_RDONLY, ENOENT);

  /* Also try opening a file with a name too long. */
  char too_long[500];
  memset(too_long, 'c', sizeof(too_long));
  too_long[sizeof(too_long) - 1] = 0;
  /* This is very unfortunate! In our errno.h: ENAMETOOLONG is 63, but errno.h
     provided by newlib (since we DID NOT port newlib to our system, which would
     include providing our custom sys/errno.h) uses ENAMETOOLONG 91. */
  assert_open_fail(too_long, 0, O_RDONLY, 63);
}

int main(int argc, char **argv) {
  test_read();
  test_devnull();
  test_multiple_descriptors();
  test_readwrite();
  test_copy();
  test_bad_descrip();
  test_open_path();

  printf("Test passed!\n");

  return 0;
}
