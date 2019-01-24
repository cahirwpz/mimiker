#include <stdio.h>

int main(int argc, char **argv) {
  printf("This is a sandbox user program. You can use it for testing and "
         "experiments in userspace. Don't commit any useful kernel tests here, "
         "please place them in utest program.\n");

  printf("argc is: %d\n", argc);
  for (int i = 0; i < argc; i++)
    printf("argv[%d] is: '%s'\n", i, argv[i]);

  return 0;
}
