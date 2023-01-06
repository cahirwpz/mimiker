#include "utest.h"

#include <stdio.h>
#include <string.h>

SET_DECLARE(tests, test_entry_t);

static test_entry_t *find_test(const char *name) {
  test_entry_t **ptr;
  SET_FOREACH (ptr, tests) {
    if (strcmp((*ptr)->name, name) == 0) {
      return *ptr;
    }
  }
  return NULL;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Not enough arguments provided to utest.\n");
    return 1;
  }

  char *test_name = argv[1];
  printf("Starting user test \"%s\".\n", test_name);

  test_entry_t *te = find_test(test_name);
  if (te)
    return te->func();

  printf("No user test \"%s\" available.\n", test_name);
  return 1;
}
