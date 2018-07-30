#include <stdio.h>
#include <stdlib.h>

extern char **environ;

int main(int argc, char **argv, char **envp) {
  printf("This is a sandbox user program. You can use it for testing and "
         "experiments in userspace. Don't commit any useful kernel tests here, "
         "please place them in utest program.\n");

  printf("-----------\n");

  printf("%p %p\n", argv, envp);
  printf("%s\n", envp[0]);
  // printf("environ pointer is: %p\n", environ);
  // printf("%s\n", environ[0]);
  environ = envp;
  printf("%s\n", getenv("DUMMY"));

  printf("-----------\n");

  return 0;
}
