#include <sys/libkern.h>
#include <sys/linker_set.h>
#include <sys/ktest.h>

static int some_int_1 = 1;
static int some_int_2 = 2;
static int some_int_3 = 3;
static int some_int_4 = 4;
static int some_int_5 = 5;

SET_ENTRY(testset, some_int_1);
SET_ENTRY(testset, some_int_2);
SET_ENTRY(testset, some_int_3);
SET_ENTRY(testset, some_int_4);
SET_ENTRY(testset, some_int_5);

static int test_linker_set(void) {
  SET_DECLARE(testset, int);

  int found[6];
  memset(found, 0, sizeof(found));

  int **ptr;
  SET_FOREACH (ptr, testset) {
    int x = **ptr;
    if (x < 0 || x > 5)
      return KTEST_FAILURE;
    found[x]++;
  }

  int key[6] = {0, 1, 1, 1, 1, 1};
  for (unsigned k = 0; k < sizeof(key) / sizeof(key[0]); k++) {
    if (found[k] != key[k])
      return KTEST_FAILURE;
  }

  return KTEST_SUCCESS;
}

KTEST_ADD(linker_set, test_linker_set, 0);
