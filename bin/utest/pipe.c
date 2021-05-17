#include "utest.h"
#include "util.h"

#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <signal.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>

// int parent_signaled_passed = 0;

// static void parent_sigpipe_handler(int signo) {
//   parent_signaled_passed = 1;
//   return;
// }

int test_pipe_parent_signaled(void) {
  // signal(SIGPIPE, &parent_sigpipe_handler);
  int pipe_fd[2];
  pid_t child_pid;

  if (pipe2(pipe_fd, 0) < 0) {
    perror("pipe2");
    exit(EXIT_FAILURE);
  }
  int wstatus;
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

  if (waitpid(child_pid, &wstatus, 0) < 0)
    perror("waitpid");

  // should trigger signal
  write(pipe_fd[1], "hello world\n", 12);

  // assert(parent_signaled_passed);
  // signal(SIGPIPE, SIG_DFL);

  return;
}

int test_pipe_child_signaled(void) {
  int child_signaled_passed = 0;
  int pipe_fd[2];
  pid_t child_pid;

  if (pipe2(pipe_fd, 0) < 0) {
    perror("pipe2");
    exit(EXIT_FAILURE);
  }
  int wstatus;
  switch (child_pid = fork()) {
    case -1: /* error */
      perror("fork error\n");
      exit(EXIT_FAILURE);

    case 0:              /* child */
      close(pipe_fd[0]); /* closing read end of pipe */
      while (1) {
        write(pipe_fd[1], "hello world\n", 12);
      }

    default:             /* parent */
      close(pipe_fd[1]); /* closing write end of pipe */
      close(pipe_fd[0]); /* closing read end of pipe */
  }
  if (waitpid(child_pid, &wstatus, 0) < 0) {
    perror("waitpid");
    exit(EXIT_FAILURE);
  }
  if (WIFSIGNALED(wstatus)) {
    child_signaled_passed = WTERMSIG(wstatus) == SIGPIPE;
  }
  assert(child_signaled_passed);

  return child_signaled_passed;
}

int test_pipe_perror(void) {
  int pipe_fd[2];
  pid_t child_pid;

  if (pipe2(pipe_fd, 0) < 0) {
    perror("pipe2");
    exit(EXIT_FAILURE);
  }
  int wstatus;
  switch (child_pid = fork()) {
    case -1: /* error */
      perror("fork\n");
      exit(EXIT_FAILURE);

    case 0:              /* child */
      close(pipe_fd[1]); /* closing write end of pipe */
      close(pipe_fd[0]); /* closing read end of pipe */
      exit(EXIT_SUCCESS);

    default:             /* parent */
      close(pipe_fd[0]); /* closing read end of pipe */
  }

  sigset_t pipe_mask;
  if ((sigemptyset(&pipe_mask) == -1) || (sigaddset(&pipe_mask, SIGPIPE) == -1))
    perror("sigemptyset or sigaddset");

  if (sigprocmask(SIG_BLOCK, &pipe_mask, NULL) < 0)
    perror("sigprocmask");

  waitpid(child_pid, &wstatus, 0);

  write(pipe_fd[1], "hello world\n", 12);
  assert(errno == EPIPE);
  sigset_t mask;
  if (sigdelset(&pipe_mask, SIGPIPE) < 0)
    perror("sigdelset");
  if (sigprocmask(SIG_SETMASK, &mask, NULL) < 0)
    perror("sigprocmask");

  return errno == EPIPE;
}
