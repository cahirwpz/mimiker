#include "utest.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

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

static timeval_t timestamp(void) {
  timespec_t ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (timeval_t){.tv_sec = ts.tv_sec, .tv_usec = ts.tv_nsec / 1000};
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Not enough arguments provided to utest.\n");
    return 1;
  }

  char *test_name = argv[1];
  timeval_t tv = timestamp();
  printf("[%d.%06d] Begin '%s' test.\n", (int)tv.tv_sec, tv.tv_usec, test_name);

  test_entry_t *te = find_test(test_name);
  if (te)
    return te->func();

  printf("No user test \"%s\" available.\n", test_name);
  return 1;
}
