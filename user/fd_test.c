#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

int main(int argc, char **argv) {
  const char *str = "Hello world from a user program!\n";
  int error = 0;
  int fd0, fd1, fd2;
  char buf[100];

  /* Open a /dev/null */
  fd0 = open("/dev/null", 0, O_WRONLY);
  assert(fd0 == 0);
  /* Open an invalid file */
  fd1 = open("/proc/cpuinfo", 0, NULL);
  assert(fd1 < 0);
  assert(errno == ENOENT);
  /* Write to /dev/null */
  error = write(fd0, str, strlen(str));
  assert(error >= 0);
  /* Open another /dev/null */
  fd1 = open("/dev/null", 0, O_RDONLY);
  assert(fd1 == 1);
  /* And one more. */
  fd2 = open("/dev/null", 0, O_RDONLY);
  assert(fd2 == 2);
  /* Read from the second /dev/null */
  memset(buf, -1, 100);
  error = read(fd1, buf, 100);
  assert(error >= 0);
  assert(buf[10] == 0 && buf[90] == 0);
  /* Close the second /dev/null */
  error = close(fd1);
  assert(error >= 0);
  /* Open one more /dev/null, observe how fd 1 is reused. */
  fd1 = open("/dev/null", 0, O_WRONLY);
  assert(fd1 == 1);
  /* Write to the newest /dev/null */
  error = write(fd0, str, strlen(str));
  assert(error >= 0);
  /* Close fd0 */
  error = close(fd0);
  assert(error >= 0);
  /* Try closing fd0 again. */
  error = close(fd0);
  assert(error < 0);
  assert(errno == EBADF);
  /* Try writing to fd0 */
  error = write(fd0, str, strlen(str));
  assert(error < 0);
  assert(errno == EBADF);
  /* Try reading from fd0 */
  memset(buf, -1, 100);
  error = read(fd0, buf, 100);
  assert(error < 0);
  assert(errno == EBADF);
  assert(buf[10] == -1 && buf[90] == -1);
  /* Try writing to file we never referenced. */
  error = write(42, str, strlen(str));
  assert(error < 0);
  assert(errno == EBADF);
  /* Try closing a file we never referenced. */
  error = close(42);
  assert(error < 0);
  assert(errno == EBADF);
  /* Try closing invalid file */
  error = close(-15);
  assert(error < 0);
  assert(errno == EBADF);
  /* At this point, fd1 and fd2 are still open. */
  /* fd1 is open in write-only mode, reading from it should fail */
  memset(buf, -1, 100);
  error = read(fd1, buf, 100);
  assert(error < 0);
  assert(errno == EBADF);
  assert(buf[10] == -1 && buf[90] == -1);
  /* fd2 is open in read-only mode, writing to it should fail */
  error = write(fd2, str, strlen(str));
  assert(error < 0);
  assert(errno == EBADF);
  /* Try opening a valid file with an invalid mode */
  error = open("/dev/null", 0, 42);
  assert(error < 0);
  assert(errno == EINVAL);
  /* Close fd1 and reopen it in read-write mode */
  close(fd1);
  fd0 = open("/dev/null", 0, O_RDWR);
  assert(fd0 == 0);
  /* See that both reading from and writing to fd0 now works. */
  memset(buf, -1, 100);
  error = read(fd0, buf, 100);
  assert(error >= 0);
  assert(buf[10] == 0 && buf[90] == 0);
  error = write(fd0, str, strlen(str));
  assert(error >= 0);

  /* Try opening a file with a name too long. */
  char too_long[500];
  memset(too_long, 'c', sizeof(too_long));
  too_long[sizeof(too_long) - 1] = 0;
  error = open(too_long, 0, O_RDWR);
  assert(error < 0);
  /* This is very unfortunate! In our errno.h: ENAMETOOLONG is 63, but errno.h
     provided by newlib (since we DID NOT port newlib to our system, which would
     include providing our custom sys/errno.h) uses ENAMETOOLONG 91. */
  assert(errno == 63);
  /* Now, try passing some invalid pointers as data to read/write. */
  /* Kernel space */
  char *naughty_ptr1 = (char *)0x80001000;
  /* User space, hopefully not mapped */
  char *naughty_ptr2 = (char *)0x00001000;
  error = write(fd0, naughty_ptr1, 200);
  assert(error < 0);
  assert(errno == EFAULT);
  error = write(fd0, naughty_ptr2, 200);
  assert(error < 0);
  assert(errno == EFAULT);
  error = read(fd0, naughty_ptr1, 200);
  assert(error < 0);
  assert(errno == EFAULT);
  error = read(fd0, naughty_ptr2, 200);
  assert(error < 0);
  assert(errno == EFAULT);
  /* Also, try opening a file using a bad pointer */
  error = open(naughty_ptr1, 0, O_RDWR);
  assert(error < 0);
  assert(errno == EFAULT);
  error = open(naughty_ptr2, 0, O_RDWR);
  assert(error < 0);
  assert(errno == EFAULT);

  /* At this point fd0 and fd2 are left open. */
  /* After this thread returns, all files should get closed. */
  return 0;
}
