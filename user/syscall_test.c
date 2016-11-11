#include <stddef.h>

int write(int fd, const char *buf, size_t count) {
  int retval, errval;
  asm volatile("move $a0, %2\n"
               "move $a1, %3\n"
               "move $a2, %4\n"
               "li $v0, 5\n" /* SYS_WRITE */
               "syscall\n"
               "move %0, $v0\n"
               "move %1, $v1\n"
               : "=r"(retval), "=r"(errval)
               : "r"(fd), "r"(buf), "r"(count)
               : "4", "5", "6", "2", "3");
  /* TODO: */
  /* errno = errval */
  return retval;
}

int main(int argc, char **argv) {
  const char *str = "Hello world from a user program!\n";
  write(1, str, 33);
  while (1)
    ;
}
