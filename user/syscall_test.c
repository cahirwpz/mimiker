#include <stddef.h>

int write(int fd, const char *buf, size_t count) {
  int retval;
  asm volatile("move $a0, %1\n"
               "move $a1, %2\n"
               "move $a2, %3\n"
               "li $v0, 5\n" /* SYS_WRITE */
               "syscall\n"
               "move %0, $v0\n"
               : "=r"(retval)
               : "r"(fd), "r"(buf), "r"(count)
               : "4", "5", "6", "2", "3");
  /* TODO: */
  /* if(retval < 0) errno = -retval */
  return retval;
}

int main(int argc, char **argv) {
  const char *str = "Hello world from a user program!\n";
  write(1, str, 33);
  while (1)
    ;
}
