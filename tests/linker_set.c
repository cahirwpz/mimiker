#include <stdc.h>
#include <linker_set.h>
#include <test.h>

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

int test_linker_set() {
  SET_DECLARE(testset, int);

  kprintf("# of elements in testset: %zu\n", SET_COUNT(testset));

  int **ptr;
  SET_FOREACH(ptr, testset) {
    kprintf("Int found in testset: %d\n", **ptr);
  }
  return 0;
}

TEST_ADD(linker_set, test_linker_set);
