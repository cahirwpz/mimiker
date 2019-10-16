#include <sys/wait.h>
#include <sys/resource.h>

pid_t waitpid(pid_t pid, int *status, int options) {
  return wait4(pid, status, options, NULL);
}
