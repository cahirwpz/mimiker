#include <pool_alloc.h>
#include <stdc.h>
#include <malloc.h>
#include <ktest.h>

void int_constr(void *buf, __attribute__((unused)) size_t size) {
  int *num = buf;
  *num = 0;
}
void int_destr(__attribute__((unused)) void *buf,
               __attribute__((unused)) size_t size) {
}

int test_pool_alloc() {
  pool_t test;
  vm_page_t *page = pm_alloc(1);

  MALLOC_DEFINE(mp, "memory pool for testing pooled memory allocator");

  kmalloc_init(mp);
  kmalloc_add_arena(mp, page->vaddr, PAGESIZE);

  for (int n = 1; n < 10; n++) {
    for (size_t size = 8; size <= 128; size += 8) {
      pool_init(&test, size, int_constr, int_destr);
      void **item = kmalloc(mp, sizeof(void *) * n, 0);
      for (int i = 0; i < n; i++) {
        item[i] = pool_alloc(&test, 0);
      }
      /* memset(item[0], 0, 100); WARNING! This line of code causes memory
      corruption, uncomment at your own risk! */
      for (int i = 0; i < n; i++) {
        pool_free(&test, item[i]);
      }
      /* pool_free(&test, item[n/2]); WARNING! This will obviously crash the
       program due to double free, uncomment at your own risk!*/
      pool_destroy(&test);
      /* pool_destroy(&test); WARNING! This will obviously crash the program
       due to double free, uncomment at your own risk!*/
      kfree(mp, item);
      kprintf("Pool allocator test passed!(n=%d, size=%d)\n", n, size);
    }
  }

  pm_free(page);
  return KTEST_SUCCESS;
}

KTEST_ADD(pool_alloc, test_pool_alloc, 0);