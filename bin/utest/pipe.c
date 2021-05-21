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
  pid_t child_pid;

  if (pipe2(pipe_fd, 0) < 0)
    perror("pipe2");

  switch (child_pid = fork()) {
    case -1: /* error */
      perror("fork");
      exit(EXIT_FAILURE);

    case 0:              /* child */
      close(pipe_fd[1]); /* closing write end of pipe */
      close(pipe_fd[0]); /* closing read end of pipe */
      exit(EXIT_SUCCESS);

    default:             /* parent */
      close(pipe_fd[0]); /* closing read end of pipe */
  }

  // Sync with end of child execution
  int wstatus;
  if (waitpid(child_pid, &wstatus, 0) < 0)
    perror("waitpid");

  // This is supposed to trigger SIGPIPE and get us killed
  write(pipe_fd[1], "hello world\n", 12);

  return 0;
}

int test_pipe_child_signaled(void) {
  int child_signaled_passed = 0;
  int pipe_fd[2];
  pid_t child_pid;

  if (pipe2(pipe_fd, 0) < 0)
    perror("pipe2");

  switch (child_pid = fork()) {
    case -1: /* error */
      perror("fork error\n");
      exit(EXIT_FAILURE);

    case 0:              /* child */
      close(pipe_fd[0]); /* closing read end of pipe */
      while (1) {
        // This is supposed to trigger deadly SIGPIPE
        write(pipe_fd[1], "hello world\n", 12);
      }

    default:             /* parent */
      close(pipe_fd[1]); /* closing write end of pipe */
      close(pipe_fd[0]); /* closing read end of pipe */
  }

  // Wait for child to die
  int wstatus;
  if (waitpid(child_pid, &wstatus, 0) < 0)
    perror("waitpid");

  // Check if child died because of SIGPIPE
  if (WIFSIGNALED(wstatus)) {
    child_signaled_passed = WTERMSIG(wstatus) == SIGPIPE;
  }
  assert(child_signaled_passed);

  return child_signaled_passed;
}

int test_pipe_perror(void) {
  int pipe_fd[2];
  pid_t child_pid;

  if (pipe2(pipe_fd, 0) < 0)
    perror("pipe2");
  switch (child_pid = fork()) {
    case -1:
      perror("fork\n");
      exit(EXIT_FAILURE);

    case 0:              /* child */
      close(pipe_fd[1]); /* closing write end of pipe */
      close(pipe_fd[0]); /* closing read end of pipe */
      exit(EXIT_SUCCESS);

    default:             /* parent */
      close(pipe_fd[0]); /* closing read end of pipe */
  }

  // Block only SIGPIPE
  sigset_t pipe_mask;
  if ((sigemptyset(&pipe_mask) == -1) || (sigaddset(&pipe_mask, SIGPIPE) == -1))
    perror("sigemptyset or sigaddset");
  if (sigprocmask(SIG_BLOCK, &pipe_mask, NULL) < 0)
    perror("sigprocmask");

  // Sync with end of child execution
  int wstatus;
  waitpid(child_pid, &wstatus, 0);

  // This should generate EPIPE, because SIGPIPE is blocked
  write(pipe_fd[1], "hello world\n", 12);
  assert(errno == EPIPE);

  // After test we drop the mask
  sigset_t mask;
  if (sigdelset(&pipe_mask, SIGPIPE) < 0)
    perror("sigdelset");
  if (sigprocmask(SIG_SETMASK, &mask, NULL) < 0)
    perror("sigprocmask");

  return errno == EPIPE;
}
