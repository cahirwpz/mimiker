#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/wait.h>

int main() {
  printf("Fork test.\n");

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
