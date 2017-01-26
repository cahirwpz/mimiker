#include <stdc.h>
#include "../include/ktest.h"

typedef struct strtol_test {
  const char *str;
  long value;
  long base;
} strtol_test_t;

strtol_test_t values[] = {
  {"123", 123, 10}, {"0xA23B", 41531, 16}, {"03417", 1807, 8}};

int test_strtol(void) {
  int max = sizeof(values) / sizeof(strtol_test_t);
  for (int i = 0; i < max; i++)
    if (values[i].value !=
        strtol(values[i].str, (char **)NULL, values[i].base)) {
      log("Mismatch for test %d: Expected: %ld, Actual: %ld", i,
          values[i].value,
          strtol(values[i].str, (char **)NULL, values[i].base));
      return KTEST_FAILURE;
    } else {
      log("Match for test %d: Expected: %ld, Actual: %ld", i, values[i].value,
          strtol(values[i].str, (char **)NULL, values[i].base));
    }

  return KTEST_SUCCESS;
}

KTEST_ADD(strtol, test_strtol);
