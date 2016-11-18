#include <stddef.h>
#include <errno.h>
#include <unistd.h>

int main(int argc, char **argv) {
  const char *str = "Hello world from a user program!\n";
  /* Successful syscall */
  write(1, str, 33);
  /* Unimplemented syscall */
  sbrk(100);
  /* assert(errno = 0); */
  while (1)
    ;
}
