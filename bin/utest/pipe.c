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

static sig_atomic_t parent_signal_delivered = 0;
static sig_atomic_t child_signal_delivered = 0;

void parent_sigpipe_handler(int signo) {
  parent_signal_delivered = 1;
  return;
}

void child_sigpipe_handler(int signo) {
  child_signal_delivered = 1;
  return;
}

int test_pipe_parent_signaled(void) {
  int pipe_fd[2];

  assert(pipe2(pipe_fd, 0) == 0);

  pid_t child_pid = fork();
  assert(child_pid > 0);

  if (child_pid == 0) { /* child */
    close(pipe_fd[1]);  /* closing write end of pipe */
    close(pipe_fd[0]);  /* closing read end of pipe */
    exit(EXIT_SUCCESS);
  }

  /* parent */
  close(pipe_fd[0]); /* closing read end of pipe */

  // Sync with end of child execution
  wait_for_child_exit(child_pid, EXIT_SUCCESS);

  // This is supposed to trigger SIGPIPE and return EPIPE
  assert(write(pipe_fd[1], "hello world\n", 12) == EPIPE);
  assert(errno == EPIPE);
  assert(parent_signal_delivered);

  return 0;
}

int test_pipe_child_signaled(void) {
  int child_signaled_passed = 0;
  int pipe_fd[2];
  pid_t child_pid;

  assert(pipe2(pipe_fd, 0) == 0);

  pid_t child_pid = fork();
  assert(child_pid > 0);

  if (child_pid == 0) { /* child */
    signal(SIGPIPE, child_sigpipe_handler);

    close(pipe_fd[0]);        /* closing read end of pipe */
    wait_for_signal(SIGUSR1); /* now we know that other end is closed */

    /* This is supposed to trigger SIGPIPE and return EPIPE */
    assert(write(pipe_fd[1], "hello world\n", 12) == EPIPE);
    assert(errno == EPIPE);
    assert(child_signal_delivered);

    exit(EXIT_SUCCESS);
  }

  /* parent */
  close(pipe_fd[1]); /* closing write end of pipe */
  close(pipe_fd[0]); /* closing read end of pipe */

  /* send SIGUSR1 informing that parent closed both ends of pipe */
  kill(child_pid, SIGUSR1);

  wait_for_child_exit(child_pid, EXIT_SUCCESS);

  return 0;
}
