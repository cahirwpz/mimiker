#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "utest.h"

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
