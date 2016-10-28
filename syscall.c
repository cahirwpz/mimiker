#include <stdc.h>
#include <sysent.h>

int main() {
  /* System call from kernel space! */
  kprintf("syscall(1) = %d\n", syscall(1));
  return 0;
}
