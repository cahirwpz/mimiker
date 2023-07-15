#include "utest.h"

#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

TEST_ADD(fork_wait) {
  int n = fork();
  if (n == 0) {
    debug("This is child, my pid is %d!", getpid());
    exit(42);
  } else {
    debug("This is parent, my pid is %d, I was told child is %d!", getpid(), n);
    int status, exitcode;
    int p = wait(&status);
    assert(WIFEXITED(status));
    exitcode = WEXITSTATUS(status);
    debug("Child exit status is %d, exit code %d.", status, exitcode);
    assert(exitcode == 42);
    assert(p == n);
  }
  return 0;
}

static volatile int done = 0;

static void sigchld_handler(int signo) {
  debug("SIGCHLD handler!");
  int n = 0;
  while ((n = waitpid(-1, NULL, WNOHANG)) > 0) {
    debug("Reaped a child.");
    done = 1;
  }
}

TEST_ADD(fork_signal) {
  signal(SIGCHLD, sigchld_handler);
  int n = fork();
  if (n == 0)
    exit(0);

  /* Wait for the child to get reaped by signal handler. */
  while (!done)
    sched_yield();
  signal(SIGCHLD, SIG_DFL);
  return 0;
}

TEST_ADD(fork_sigchld_ignored) {
  /* Please auto-reap my children. */
  signal(SIGCHLD, SIG_IGN);
  int n = fork();
  if (n == 0)
    exit(0);

  /* wait() should fail, since the child reaps itself. */
  assert(wait(NULL) == -1);
  return 0;
}
