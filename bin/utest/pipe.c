#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#include <signal.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "utest.h"
#include "util.h"

int test_pipe_blocking_flag_manipulation(void) {
  int pipe_fd[2];

  /* creating pipe */
  int pipe_ret_val = pipe2(pipe_fd, O_NONBLOCK);
  assert(pipe_ret_val == 0);

  /* check if flag is set */
  assert(fcntl(pipe_fd[0], F_GETFL) & O_NONBLOCK);
  assert(fcntl(pipe_fd[1], F_GETFL) & O_NONBLOCK);

  /* unset same flag for read end */
  int read_flagset_with_block;
  assert(read_flagset_with_block = fcntl(pipe_fd[0], F_GETFL) > 0);
  fcntl(pipe_fd[0], F_SETFL, read_flagset_with_block & ~O_NONBLOCK);
  /* unset same flag for write end */
  int write_flagset_with_block;
  assert(write_flagset_with_block = fcntl(pipe_fd[1], F_GETFL) > 0);
  fcntl(pipe_fd[1], F_SETFL, write_flagset_with_block & ~O_NONBLOCK);

  /* check if flag is not set */
  assert(!(fcntl(pipe_fd[0], F_GETFL) & O_NONBLOCK));
  assert(!(fcntl(pipe_fd[1], F_GETFL) & O_NONBLOCK));

  return 0;
}
