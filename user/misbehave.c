#include <stddef.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>

int main(int argc, char **argv) {
  const char str[] = "Hello world from a user program!\n";

  /* Successful syscall */
  assert(write(STDOUT_FILENO, str, sizeof(str) - 1) == sizeof(str) - 1);

  /* Pass bad pointer */
  assert(write(STDOUT_FILENO, (char *)0xDEADC0DE, 100) == -1);
  assert(errno == EFAULT);

  return 1;
}
