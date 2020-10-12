#include <unistd.h>

extern char **environ;

int execv(const char *path, char *const *argv) {
  return execve(path, argv, environ);
}
