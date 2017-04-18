#include <pool_alloc.h>
#include <stdc.h>
#include <malloc.h>
#include <ktest.h>

void int_constr(void *buf, __unused size_t size) {
  int *num = buf;
  *num = 0;
}

typedef enum {
  PALLOC_TEST_REGULAR,
  PALLOC_TEST_CORRUPTION,
  PALLOC_TEST_DOUBLEFREE
} palloc_test_t;

int test_pool_alloc(
  palloc_test_t flag) { /* PALLOC_TEST_REGULAR - regular test,
                           PALLOC_TEST_CORRUPTION - memory corrruption,
                           PALLOC_TEST_DOUBLEFREE - double free */

  pool_t test;

  MALLOC_DEFINE(mp, "memory pool for testing pooled memory allocator");

  kmalloc_init(mp, 1, 1);

  for (int n = 1; n < 10; n++) {
    for (size_t size = 4; size <= 128; size += 4) {
      pool_init(&test, size, int_constr, pool_default_destructor);
      void **item = kmalloc(mp, sizeof(void *) * n, 0);
      for (int i = 0; i < n; i++) {
        item[i] = pool_alloc(&test, 0);
      }
      if (flag == PALLOC_TEST_CORRUPTION)
        memset(item[0], 0, 100); /* WARNING! This line of code causes memory
corruptio! */
      for (int i = 0; i < n; i++) {
        pool_free(&test, item[i]);
      }
      if (flag == PALLOC_TEST_DOUBLEFREE)
        pool_free(&test, item[n / 2]); /*WARNING! This will obviously crash the
program due to double free! */
      pool_destroy(&test);
      /* pool_destroy(&test); WARNING! This will obviously crash the program
       due to double free, uncomment at your own risk!*/
      kfree(mp, item);
      kprintf("Pool allocator test passed!(n=%d, size=%d)\n", n, size);
    }
  }

  return KTEST_SUCCESS;
}

int test_pool_alloc_regular() {
  return test_pool_alloc(PALLOC_TEST_REGULAR);
}

int test_pool_alloc_corruption() {
  return test_pool_alloc(PALLOC_TEST_CORRUPTION);
}

int test_pool_alloc_doublefree() {
  return test_pool_alloc(PALLOC_TEST_DOUBLEFREE);
}

KTEST_ADD(pool_alloc_regular, test_pool_alloc_regular, 0);
KTEST_ADD(pool_alloc_corruption, test_pool_alloc_corruption, KTEST_FLAG_BROKEN);
KTEST_ADD(pool_alloc_doublefree, test_pool_alloc_doublefree, KTEST_FLAG_BROKEN);
