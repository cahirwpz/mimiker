#include <stdio.h>

int main(int argc, char **argv, char **envp) {

  printf(
    "I will dump arguments and environment variables passed in commandline.\n");
  printf("Argument count is %d\n", argc);

  int i;

  if (argv != NULL) {
    for (i = 0; i < argc; i++) {
      if (argv[i] != NULL)
        printf("Argument No. %d is: %s\n", i, argv[i]);
      else
        printf("Argument No. %d is NULL\n", i);
    }
  } else
    printf("Argument list argv is NULL\n");

  printf("Success!.\n");

  return 0;
}
