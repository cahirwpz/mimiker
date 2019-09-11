#include <unistd.h>

pid_t vfork(void) {
  return fork();
}
