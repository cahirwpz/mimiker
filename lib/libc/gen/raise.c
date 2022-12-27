#include <signal.h>
#include <unistd.h>

int raise(int s) {
  return kill(getpid(), s);
}
