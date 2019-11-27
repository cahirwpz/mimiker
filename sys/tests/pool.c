#include <sys/libkern.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/ktest.h>

typedef enum {
  PALLOC_TEST_REGULAR,    /* regular test */
  PALLOC_TEST_CORRUPTION, /* memory corruption */
  PALLOC_TEST_DOUBLEFREE, /* double free */
} palloc_test_t;

static int test_pool_alloc(palloc_test_t flag) {
  const int N = 50;

  kmalloc_pool_t *mp = kmalloc_create("test", PAGESIZE * 2);

  int size = 64;
  pool_t *test = pool_create("test", size);

  for (int n = 1; n < N; n++) {
    void **item = kmalloc(mp, sizeof(void *) * n, 0);
    for (int i = 0; i < n; i++)
      item[i] = pool_alloc(test, PF_ZERO);
    if (flag == PALLOC_TEST_CORRUPTION) {
      /* WARNING! Following line of code causes memory corruption! */
      memset(item[0], 0, 100);
    }
    for (int i = 0; i < n; i++)
      pool_free(test, item[i]);
    if (flag == PALLOC_TEST_DOUBLEFREE) {
      /* WARNING! This will obviously crash kernel due to double free! */
      pool_free(test, item[n / 2]);
    }
    kfree(mp, item);
  }
  pool_destroy(test);
#if 0
  /* WARNING! This will obviously crash the program due to double free,
   * uncomment at your own risk! */
  pool_destroy(&test);
#endif
  kmalloc_destroy(mp);
  return KTEST_SUCCESS;
}

static int test_pool_alloc_regular(void) {
  return test_pool_alloc(PALLOC_TEST_REGULAR);
}

static int test_pool_alloc_corruption(void) {
  return test_pool_alloc(PALLOC_TEST_CORRUPTION);
}

static int test_pool_alloc_doublefree(void) {
  return test_pool_alloc(PALLOC_TEST_DOUBLEFREE);
}

KTEST_ADD(pool_alloc_regular, test_pool_alloc_regular, 0);
KTEST_ADD(pool_alloc_corruption, test_pool_alloc_corruption, KTEST_FLAG_BROKEN);
KTEST_ADD(pool_alloc_doublefree, test_pool_alloc_doublefree, KTEST_FLAG_BROKEN);
