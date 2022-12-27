#include <sys/wait.h>

pid_t wait(int *status) {
  return wait4(-1, status, 0, NULL);
}

pid_t wait3(int *status, int options, struct rusage *rusage) {
  return wait4(-1, status, options, rusage);
}
