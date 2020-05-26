#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sched.h>

int test_fork_wait(void) {
  int n = fork();
  if (n == 0) {
    printf("This is child, my pid is %d!\n", getpid());
    exit(42);
  } else {
    printf("This is parent, my pid is %d, I was told child is %d!\n", getpid(),
           n);
    int status, exitcode;
    int p = wait(&status);
    assert(WIFEXITED(status));
    exitcode = WEXITSTATUS(status);
    printf("Child exit status is %d, exit code %d.\n", status, exitcode);
    assert(exitcode == 42);
    assert(p == n);
  }
  return 0;
}

volatile int done = 0;

void sigchld_handler(int signo) {
  printf("SIGCHLD handler!\n");
  int n = 0;
  while ((n = waitpid(-1, NULL, WNOHANG)) > 0) {
    printf("Reaped a child.\n");
    done = 1;
  }
}

int test_fork_signal(void) {
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

int test_fork_sigchld_ignored(void) {
  /* Please auto-reap my children. */
  signal(SIGCHLD, SIG_IGN);
  int n = fork();
  if (n == 0)
    exit(0);

  /* wait() should fail, since the child reaps itself. */
  assert(wait(NULL) == -1);
  return 0;
}
