#include <signal.h>

int sigemptyset(sigset_t *set) {
  __sigemptyset(set);
  return 0;
}
