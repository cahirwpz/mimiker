#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <signal.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "utest.h"
#include "util.h"

int test_pipe_parent_signaled(void) {
  int pipe_fd[2];

  /* creating pipe */
  assert(pipe2(pipe_fd, O_NONBLOCK) == 0);

  /* forking */
  pid_t child_pid = fork();
  assert(child_pid >= 0);

  if (child_pid == 0) { /* child */
  }
}
