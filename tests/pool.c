#include <stdc.h>
#include <malloc.h>
#include <pool.h>
#include <ktest.h>

typedef enum {
  PALLOC_TEST_REGULAR,    /* regular test */
  PALLOC_TEST_CORRUPTION, /* memory corruption */
  PALLOC_TEST_DOUBLEFREE, /* double free */
} palloc_test_t;

static void int_ctor(void *buf, __unused size_t size) {
  int *num = buf;
  *num = 0;
}

static int test_pool_alloc(palloc_test_t flag) {
  pool_t test;

  MALLOC_DEFINE(mp, "memory pool for testing pooled memory allocator");

  kmalloc_init(mp, 1, 1);

  for (int n = 1; n < 10; n++) {
    for (size_t size = 4; size <= 128; size += 4) {
      pool_init(&test, size, int_ctor, NULL);
      void **item = kmalloc(mp, sizeof(void *) * n, 0);
      for (int i = 0; i < n; i++)
        item[i] = pool_alloc(&test, 0);
      if (flag == PALLOC_TEST_CORRUPTION) {
        /* WARNING! Following line of code causes memory corruption! */
        memset(item[0], 0, 100);
      }
      for (int i = 0; i < n; i++)
        pool_free(&test, item[i]);
      if (flag == PALLOC_TEST_DOUBLEFREE) {
        /* WARNING! This will obviously crash kernel due to double free! */
        pool_free(&test, item[n / 2]);
      }
      pool_destroy(&test);
#if 0
      /* WARNING! This will obviously crash the program due to double free,
       * uncomment at your own risk! */
      pool_destroy(&test);
#endif
      kfree(mp, item);
      kprintf("Pool allocator test passed!(n=%d, size=%d)\n", n, size);
    }
  }

  return KTEST_SUCCESS;
}

static int test_pool_alloc_regular() {
  return test_pool_alloc(PALLOC_TEST_REGULAR);
}

static int test_pool_alloc_corruption() {
  return test_pool_alloc(PALLOC_TEST_CORRUPTION);
}

static int test_pool_alloc_doublefree() {
  return test_pool_alloc(PALLOC_TEST_DOUBLEFREE);
}

KTEST_ADD(pool_alloc_regular, test_pool_alloc_regular, 0);
KTEST_ADD(pool_alloc_corruption, test_pool_alloc_corruption, KTEST_FLAG_BROKEN);
KTEST_ADD(pool_alloc_doublefree, test_pool_alloc_doublefree, KTEST_FLAG_BROKEN);
