#include <stdio.h>
#include <unistd.h>

int main() {
  printf("Fork test.\n");

  int n = fork();
  if (n == 0) {
    printf("This is child, my pid is %d!\n", getpid());
  } else {
    printf("This is parent, my pid is %d, I was told child is %d!\n", getpid(),
           n);
  }

  return 0;
}
