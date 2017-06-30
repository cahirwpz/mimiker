#include <stdio.h>

int main(int argc, char **argv, char **envp) {

  printf(
    "I will dump arguments and environment variables passed in commandline.\n");
  printf("Argument count is %d\n", argc);

  size_t i;

  if (argv != NULL) {
    for (i = 0; i < argc; i++) {
      if (argv[i] != NULL)
        printf("Argument No. %d is: %s\n", i, argv[i]);
      else
        printf("Argument No. %d is NULL\n", i);
    }
  } else
    printf("Argument list argv is NULL\n");

  /* if (envp != NULL) { */

  /*   i = 0; */
  /*   while (envp[i] != NULL) i++; */
  /*   printf("Environment count is %d\n", i); */

  /*   i = 0; */
  /*   while (envp[i] != NULL) { */
  /*     printf("Argument No. %d is: %s\n", i, argv[i]); */
  /*     i++; */
  /*   } */

  /* } else */
  /*   printf("Environment envp is NULL\n"); */

  printf("Success!.\n");

  return 0;
}
