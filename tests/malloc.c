#include <stdc.h>
#include <malloc.h>
#include <ktest.h>
#include <thread.h>
#include <sched.h>

static int malloc_one_allocation(void) {
  kmem_pool_t *mp = KMEM_POOL("test", 1, 1);
  kmem_init(mp);
  void *ptr = kmalloc(mp, 1234, M_NOWAIT);
  assert(ptr != NULL);
  kfree(mp, ptr);
  kmem_destroy(mp);
  return KTEST_SUCCESS;
}

static int malloc_invalid_values(void) {
  kmem_pool_t *mp = KMEM_POOL("test", 1, 1);
  kmem_init(mp);
  void *ptr = kmalloc(mp, PAGESIZE, M_NOWAIT);
  assert(ptr == NULL);
  ptr = kmalloc(mp, 0, M_NOWAIT);
  assert(ptr == NULL);
  kmem_destroy(mp);
  return KTEST_SUCCESS;
}

static int malloc_multiple_allocations(void) {
  kmem_pool_t *mp = KMEM_POOL("test", 1, 1);
  kmem_init(mp);
  const int n = 50;
  void *ptrs[n];
  for (int i = 0; i < n; i++) {
    ptrs[i] = kmalloc(mp, i + 1, M_NOWAIT);
    assert(NULL != ptrs[i]);
  }
  for (int i = 0; i < n; i++)
    kfree(mp, ptrs[i]);
  kmem_destroy(mp);
  return KTEST_SUCCESS;
}

static int malloc_dynamic_pages_addition(void) {
  kmem_pool_t *mp = KMEM_POOL("test", 1, 3);
  kmem_init(mp);
  void *ptr1 = kmalloc(mp, 4000, 0);
  assert(ptr1 != NULL);
  void *ptr2 = kmalloc(mp, 4000, 0);
  assert(ptr2 != NULL);
  void *ptr3 = kmalloc(mp, 4000, 0);
  assert(ptr3 != NULL);
  kfree(mp, ptr1);
  kfree(mp, ptr2);
  kfree(mp, ptr3);
  kmem_destroy(mp);
  return KTEST_SUCCESS;
}

#define MALLOC_RANDINT_PAGES 64
static int malloc_random_size(unsigned int randint) {
  if (randint == 0)
    randint = 64;

  kmem_pool_t *mp =
    KMEM_POOL("test", MALLOC_RANDINT_PAGES, MALLOC_RANDINT_PAGES);
  kmem_init(mp);
  void *ptr = kmalloc(mp, randint, M_NOWAIT);
  assert(ptr != NULL);
  kfree(mp, ptr);
  kmem_destroy(mp);
  return KTEST_SUCCESS;
}

KTEST_ADD(malloc_one_allocation, malloc_one_allocation, 0);
KTEST_ADD(malloc_invalid_values, malloc_invalid_values, 0);
KTEST_ADD(malloc_multiple_allocations, malloc_multiple_allocations, 0);
KTEST_ADD(malloc_dynamic_pages_addition, malloc_dynamic_pages_addition, 0);
/* Reserve some memory for mem_block_t. */
#define RESERVED 1024
KTEST_ADD_RANDINT(malloc_random_size, malloc_random_size, 0,
                  MALLOC_RANDINT_PAGES *PAGESIZE - RESERVED);
