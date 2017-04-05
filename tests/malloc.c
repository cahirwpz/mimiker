#include <stdc.h>
#include <malloc.h>
#include <ktest.h>
#include <thread.h>
#include <sched.h>

static int malloc_one_allocation(void) {
  MALLOC_DEFINE(mp, "testing memory pool");
  kmalloc_init(mp, 1, 1);
  void *ptr = kmalloc(mp, 1234, M_NOWAIT);
  assert(ptr != NULL);
  kfree(mp, ptr);
  return KTEST_SUCCESS;
}

static int malloc_invalid_values(void) {
  MALLOC_DEFINE(mp, "testing memory pool");
  kmalloc_init(mp, 1, 1);
  void *ptr = kmalloc(mp, PAGESIZE, M_NOWAIT);
  assert(ptr == NULL);
  ptr = kmalloc(mp, 0, M_NOWAIT);
  assert(ptr == NULL);
  return KTEST_SUCCESS;
}

static int malloc_multiple_allocations(void) {
  MALLOC_DEFINE(mp, "testing memory pool");
  kmalloc_init(mp, 1, 1);
  const int n = 50;
  void *ptrs[n];
  for (int i = 0; i < n; i++)
    assert(NULL != (ptrs[i] = kmalloc(mp, i + 1, M_NOWAIT)));
  for (int i = 0; i < n; i++)
    kfree(mp, ptrs[i]);
  return KTEST_SUCCESS;
}

static int malloc_dynamic_pages_addition(void) {
  MALLOC_DEFINE(mp, "testing memory pool");
  kmalloc_init(mp, 1, 3);
  void *ptr1 = kmalloc(mp, 4000, 0);
  assert(ptr1 != NULL);
  void *ptr2 = kmalloc(mp, 4000, 0);
  assert(ptr2 != NULL);
  void *ptr3 = kmalloc(mp, 4000, 0);
  assert(ptr3 != NULL);
  kfree(mp, ptr1);
  kfree(mp, ptr2);
  kfree(mp, ptr3);
  return KTEST_SUCCESS;
}

#define THREADS_NUMBER 10
#define ALLOCATIONS_PER_THREAD 200
#define ALLOCATIONS (THREADS_NUMBER * ALLOCATIONS_PER_THREAD)

void *ptrs[ALLOCATIONS];
static malloc_pool_t test_pool[1] = {
  {{NULL}, MB_MAGIC, "testing memory pool", {NULL, NULL}}};

static void threads_function(void *arg) {
  const unsigned start = ALLOCATIONS_PER_THREAD * (unsigned)arg;
  for (unsigned i = 0; i < ALLOCATIONS_PER_THREAD; i++) {
    ptrs[i + start] = kmalloc(test_pool, 1, 0);
    assert(NULL != ptrs[i + start]);
  }
}

static int malloc_multiple_threads(void) {
  kmalloc_init(test_pool, 1, 40);
  thread_t *threads[THREADS_NUMBER];
  for (int i = 0; i < THREADS_NUMBER; i++)
    threads[i] =
      thread_create("Malloc test thread", threads_function, (void *)i);
  for (int i = 0; i < THREADS_NUMBER; i++)
    sched_add(threads[i]);
  for (int i = 0; i < THREADS_NUMBER; i++)
    thread_join(threads[i]);
  for (int i = 0; i < ALLOCATIONS; i++)
    kfree(test_pool, ptrs[i]);
  return KTEST_SUCCESS;
}

#define MALLOC_RANDINT_PAGES 64
static int malloc_random_size(unsigned int randint) {
  if (randint == 0)
    randint = 64;

  MALLOC_DEFINE(mp, "testing memory pool");
  kmalloc_init(mp, MALLOC_RANDINT_PAGES, MALLOC_RANDINT_PAGES);

  void *ptr = kmalloc(mp, randint, M_NOWAIT);
  assert(ptr != NULL);
  kfree(mp, ptr);
  return KTEST_SUCCESS;
}

KTEST_ADD(malloc_one_allocation, malloc_one_allocation, 0);
KTEST_ADD(malloc_invalid_values, malloc_invalid_values, 0);
KTEST_ADD(malloc_multiple_allocations, malloc_multiple_allocations, 0);
KTEST_ADD(malloc_dynamic_pages_addition, malloc_dynamic_pages_addition, 0);
KTEST_ADD(malloc_multiple_threads, malloc_multiple_threads, 0);
/* Reserve some memory for mem_block_t. */
#define RESERVED 1024
KTEST_ADD_RANDINT(malloc_random_size, malloc_random_size, 0,
                  MALLOC_RANDINT_PAGES *PAGESIZE - RESERVED);
