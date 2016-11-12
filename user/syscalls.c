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
               : "%a0", "%a1", "%a2", "%v0");
  /* Exit may not return! */
  while (1)
    ;
}

int open(const char *pathname, int flags, ...) {
  int retval;
  asm volatile("move $a0, %1\n"
               "move $a1, %2\n"
               "li $v0, 2\n" /* SYS_OPEN */
               "syscall\n"
               "move %0, $v0\n"
               : "=r"(retval)
               : "r"(pathname), "r"(flags)
               : "%a0", "%a1", "%v0");
  if (retval < 0) {
    errno = -retval;
    return -1;
  }
  return retval;
}

int close(int fd) {
  int retval;
  asm volatile("move $a0, %1\n"
               "li $v0, 3\n" /* SYS_CLOSE */
               "syscall\n"
               "move %0, $v0\n"
               : "=r"(retval)
               : "r"(fd)
               : "%a0", "%v0");
  if (retval < 0) {
    errno = -retval;
    return -1;
  }
  return retval;
}

ssize_t read(int fd, void *buf, size_t count) {
  int retval;
  asm volatile("move $a0, %1\n"
               "move $a1, %2\n"
               "move $a2, %3\n"
               "li $v0, 4\n" /* SYS_READ */
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

_off_t lseek(int fd, _off_t offset, int whence) {
  int retval;
  asm volatile("move $a0, %1\n"
               "move $a1, %2\n"
               "move $a2, %3\n"
               "li $v0, 6\n" /* SYS_LSEEK */
               "syscall\n"
               "move %0, $v0\n"
               : "=r"(retval)
               : "r"(fd), "r"(offset), "r"(whence)
               : "%a0", "%a1", "%a2", "%v0");
  if (retval < 0) {
    errno = -retval;
    return -1;
  }
  return retval;
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
  int retval;
  asm volatile("move $a0, %1\n"
               "li $v0, 7\n" /* SYS_UNLINK */
               "syscall\n"
               "move %0, $v0\n"
               : "=r"(retval)
               : "r"(path)
               : "%a0", "%v0");
  if (retval < 0) {
    errno = -retval;
    return -1;
  }
  return retval;
}

ssize_t readlink(const char *__restrict __path, char *__restrict __buf,
                 size_t __buflen) {
  errno = ENOSYS;
  return -1;
}

pid_t getpid() {
  int retval;
  asm volatile("li $v0, 8\n" /* SYS_GETPID */
               "syscall\n"
               "move %0, $v0\n"
               : "=r"(retval)
               :
               : "%v0");

  /* TODO: getpid never fails. */
  if (retval < 0) {
    errno = -retval;
    return -1;
  }
  return retval;
}

int kill(pid_t pid, int sig) {
  int retval;
  asm volatile("move $a0, %1\n"
               "move $a1, %2\n"
               "li $v0, 9\n" /* SYS_KILL */
               "syscall\n"
               "move %0, $v0\n"
               : "=r"(retval)
               : "r"(pid), "r"(sig)
               : "%a0", "%a1", "%v0");
  if (retval < 0) {
    errno = -retval;
    return -1;
  }
  return retval;
}

int fstat(int fd, struct stat *buf) {
  int retval;
  asm volatile("move $a0, %1\n"
               "move $a1, %2\n"
               "li $v0, 10\n" /* SYS_FSTAT */
               "syscall\n"
               "move %0, $v0\n"
               : "=r"(retval)
               : "r"(fd), "r"(buf)
               : "%a0", "%a1", "%a2", "%v0");
  if (retval < 0) {
    errno = -retval;
    return -1;
  }
  return retval;
}

void *sbrk(ptrdiff_t increment) {
  errno = ENOSYS;
  return (void *)-1;
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
