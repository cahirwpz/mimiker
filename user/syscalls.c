#include <errno.h>

#ifdef __NEWLIB__
/* unistd provides prototypes for syscall hooks. Including this file makes
   compiler emit errors should we implement a non-confirming hook. */
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/times.h>
#else
/* These are for the old toolchain. */
struct stat;
struct tms;
struct timeval;
#include <sys/types.h>
#endif

void _exit(int status) {
  int retval;
  asm volatile("move $a0, %1\n"
               "li $v0, 1\n" /* SYS_EXIT */
               "syscall\n"
               "move %0, $v0\n"
               : "=r"(retval)
               : "r"(status)
               : "%a0", "%v0");
  /* This should be unreachable */
  while (1)
    ;
}

int open(const char *pathname, int flags, ...) {
  errno = ENOSYS;
  return -1;
}

int close(int __fildes) {
  errno = ENOSYS;
  return -1;
}

ssize_t read(int __fd, void *__buf, size_t __nbyte) {
  errno = ENOSYS;
  return -1;
}

ssize_t write(int fd, const void *buf, size_t count) {
  int retval;
  asm volatile("move $a0, %1\n"
               "move $a1, %2\n"
               "move $a2, %3\n"
               "li $v0, 5\n" /* SYS_WRITE */
               "syscall\n"
               "move %0, $v0\n"
               : "=r"(retval)
               : "r"(fd), "r"(buf), "r"(count)
               : "%a0", "%a1", "%a2", "%v0");
  if (retval < 0) {
    errno = -retval;
    return -1;
  }
  return retval;
}

_off_t lseek(int __fildes, _off_t __offset, int __whence) {
  errno = ENOSYS;
  return (off_t)-1;
}

int link(const char *__path1, const char *__path2) {
  errno = ENOSYS;
  return -1;
}

int symlink(const char *__name1, const char *__name2) {
  errno = ENOSYS;
  return -1;
}

int unlink(const char *path) {
  errno = ENOSYS;
  return -1;
}

ssize_t readlink(const char *__restrict __path, char *__restrict __buf,
                 size_t __buflen) {
  errno = ENOSYS;
  return -1;
}

pid_t getpid() {
  /* getpid never fails. */
  return 0;
}

int kill(pid_t pid, int sig) {
  errno = ENOSYS;
  return -1;
}

int fstat(int fd, struct stat *buf) {
  errno = ENOSYS;
  return -1;
}

void *sbrk(ptrdiff_t increment) {
  int retval;
  asm volatile("move $a0, %1\n"
               "li $v0, 11\n" /* SYS_SBRK */
               "syscall\n"
               "move %0, $v0\n"
               : "=r"(retval)
               : "r"(increment)
               : "%a0", "%v0");
  if (retval < 0) {
    errno = -retval;
    return (void *)-1;
  }
  return (void *)retval;
}

int isatty(int __fildes) {
  /* isatty never fails, so just pretend nothing is a TTY. */
  errno = 0;
  return 0;
}

pid_t fork() {
  errno = ENOSYS;
  return -1;
}

int execve(const char *__path, char *const __argv[], char *const __envp[]) {
  errno = ENOSYS;
  return -1;
}

pid_t wait(int *status) {
  errno = ENOSYS;
  return -1;
}

clock_t times(struct tms *buf) {
  errno = ENOSYS;
  return (clock_t)-1;
}

int gettimeofday(struct timeval *__restrict __p, void *__restrict __tz) {
  errno = ENOSYS;
  return -1;
}
