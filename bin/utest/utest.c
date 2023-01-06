#include "utest.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

noreturn void utest_die(const char *file, int line, const char *func,
                        const char *expr) {
  char buf[1024];
  int l = snprintf_ss(buf, sizeof(buf),
                      "assertion \"%s\" failed: file \"%s\", line %d%s%s%s\n",
                      expr, file, line, func ? ", function \"" : "",
                      func ? func : "", func ? "\"" : "");
  if (l < 0)
    exit(EXIT_FAILURE);
  (void)write(STDERR_FILENO, buf, (size_t)l);
  exit(EXIT_FAILURE);
}

int utest_spawn(proc_func_t func, void *arg) {
  int pid;
  switch ((pid = fork())) {
    case -1:
      exit(EXIT_FAILURE);
    case 0:
      exit(func(arg));
    default:
      return pid;
  }
}

void utest_child_exited(int exitcode) {
  int status;
  wait(&status);
  assert(WIFEXITED(status));
  assert(WEXITSTATUS(status) == exitcode);
}
