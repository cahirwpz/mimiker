#include "utest.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Not enough arguments provided to utest.\n");
    return 1;
  }
  char *test_name = argv[1];
  printf("Starting user test \"%s\".\n", test_name);


#define CHECKRUN_TEST(name) \
  if (strcmp(test_name, #name) == 0) \
    return test_##name();

  /* Linker set in userspace would be quite difficult to set up, and it feels
     like an overkill to me. */
  CHECKRUN_TEST(mmap);
  CHECKRUN_TEST(sbrk);
  CHECKRUN_TEST(misbehave);
  CHECKRUN_TEST(fd_read);
  CHECKRUN_TEST(fd_devnull);
  CHECKRUN_TEST(fd_multidesc);
  CHECKRUN_TEST(fd_readwrite);
  CHECKRUN_TEST(fd_copy);
  CHECKRUN_TEST(fd_bad_desc);
  CHECKRUN_TEST(fd_open_path);
  CHECKRUN_TEST(fd_dup);
  CHECKRUN_TEST(fd_all);
  
  printf("No user test \"%s\" available.\n", test_name);
  return 1;
}
