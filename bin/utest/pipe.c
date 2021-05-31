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

static sig_atomic_t signal_delivered;

static void sigpipe_handler(int signo) {
  signal_delivered = 1;
  return;
}

int test_pipe_parent_signaled(void) {
  int pipe_fd[2];
  signal_delivered = 0;
  signal(SIGPIPE, sigpipe_handler);

  /* creating pipe */
  assert(pipe2(pipe_fd, 0) == 0);

  /* forking */
  pid_t child_pid = fork();
  assert(child_pid >= 0);

  if (child_pid == 0) { /* child */
    close(pipe_fd[1]);  /* closing write end of pipe */
    close(pipe_fd[0]);  /* closing read end of pipe */
    exit(EXIT_SUCCESS);
  }

  /* parent */
  close(pipe_fd[0]); /* closing read end of pipe */

  /* Sync with end of child execution */
  wait_for_child_exit(child_pid, EXIT_SUCCESS);

  /* This is supposed to trigger SIGPIPE and return EPIPE */
  ssize_t write_ret = write(pipe_fd[1], "hello world\n", 12);
  assert(write_ret == -1);
  assert(errno == EPIPE);
  assert(signal_delivered);

  return 0;
}

int test_pipe_child_signaled(void) {
  int pipe_fd[2];
  signal_delivered = 0;

  /* set up SIGUSR1 so it's not lethal for my child */
  signal_setup(SIGUSR1);

  /* creating pipe */
  assert(pipe2(pipe_fd, 0) == 0);

  /* forking */
  pid_t child_pid = fork();
  assert(child_pid >= 0);

  if (child_pid == 0) { /* child */
    signal(SIGPIPE, sigpipe_handler);

    close(pipe_fd[0]);        /* closing read end of pipe */
    wait_for_signal(SIGUSR1); /* now we know that other end is closed */

    /* This is supposed to trigger SIGPIPE and return EPIPE */
    assert(write(pipe_fd[1], "hello world\n", 12) == -1);
    assert(errno == EPIPE);
    assert(signal_delivered);

    exit(EXIT_SUCCESS);
  }

  /* parent */
  close(pipe_fd[1]); /* closing write end of pipe */
  close(pipe_fd[0]); /* closing read end of pipe */

  /* send SIGUSR1 informing that parent closed both ends of pipe */
  kill(child_pid, SIGUSR1);

  /* I really want child to finish, not just change it's state.
   * so i don't use wait_for_child_exit
   */
  int wstatus = 1;
  do {
    assert(waitpid(child_pid, &wstatus, 0) == child_pid);
  } while (!WIFEXITED(wstatus));

  assert(WEXITSTATUS(wstatus) == EXIT_SUCCESS);

  return 0;
}
