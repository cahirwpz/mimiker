#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/signal.h>

void fork_test_wait() {
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

void fork_test_signal() {
  signal(SIGCHLD, sigchld_handler);
  int n = fork();
  if (n == 0)
    exit(0);

  /* Wait for the child to get reaped by signal handler. */
  while (!done)
    ;
  signal(SIGCHLD, SIG_DFL);
}

void fork_test_sigchld_ignored() {
  /* Please auto-reap my children. */
  signal(SIGCHLD, SIG_IGN);
  int n = fork();
  if (n == 0)
    exit(0);

  /* TODO: Use a timeout here, because otherwise the parent process exits
     first... Which is also a useful test, but doesn't really verify an ignored
     SIGCHILD. */
}

int main() {
  printf("Testing fork/wait.\n");
  fork_test_wait();
  fork_test_signal();
  fork_test_sigchld_ignored();
  printf("Success!\n");
  return 0;
}
