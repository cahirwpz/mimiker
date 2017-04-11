#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pool_alloc.h"

void int_constr(void *buf, __attribute__((unused)) size_t size) {
  int *num = buf;
  *num = 0;
}
void int_destr(__attribute__((unused)) void *buf,
               __attribute__((unused)) size_t size) {
}

typedef struct test {
  int foo;
  char bar[200];
} test_t;

void test_constr(void *buf, __attribute__((unused)) size_t size) {
  test_t *test = buf;
  test->foo = 0;
  strcpy(test->bar, "Hello, World!");
}

void test_destr(__attribute__((unused)) void *buf,
                __attribute__((unused)) size_t size) {
}

int main() {
  for (int n = 0; n < 1000; n++) {
    for (size_t size = 8; size <= 128; size += 8) {
      pool_t test;
      pool_init(&test, size, int_constr, int_destr);
      void **item = malloc(sizeof(void *) * n);
      for (int i = 0; i < n; i++) {
        item[i] = pool_alloc(&test, 0);
      }
      for (int i = 0; i < n; i++) {
        pool_free(&test, item[i]);
      }
      pool_destroy(&test);
      fprintf(stderr, "Pool allocator test passed!(n=%d, size=%ld)\n", n, size);
    }
  }
}