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
    int exitcode = 0;
    int p = wait(&exitcode);
    printf("Child exit code is %d.\n", exitcode);
    assert(exitcode == 42);
    assert(p == n);
  }

  return 0;
}
