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
  for (int i = 0; i < n; i++) {
    ptrs[i] = kmalloc(mp, i + 1, M_NOWAIT);
    assert(NULL != ptrs[i]);
  }
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

#define ALLOCATIONS_PER_THREAD 10000
#define THREADS_NUMBER 10

static void malloc_one_block_at_a_time(void *arg) {
  for (int i = 0; i < ALLOCATIONS_PER_THREAD; i++) {
    void *ptr = kmalloc(arg, 1, 0);
    assert(ptr != NULL);
    kfree(arg, ptr);
  }
}

#define BLOCKS_AT_A_TIME 100
#define MAX_BLOCK_SIZE 128
static void malloc_many_blocks_at_a_time(void *arg) {
  void *ptrs[BLOCKS_AT_A_TIME];
  for (int i = 0; i < BLOCKS_AT_A_TIME; i++) {
    ptrs[i] = kmalloc(arg, 1, 0);
    assert(ptrs[i] != NULL);
  }
  for (int i = BLOCKS_AT_A_TIME; i < ALLOCATIONS_PER_THREAD; i++) {
    int idx = i % BLOCKS_AT_A_TIME;
    kfree(arg, ptrs[idx]);
    ptrs[idx] = kmalloc(arg, 1, 0);
    assert(ptrs[idx] != NULL);
  }
}

static void malloc_multithreaded(void (*threads_function)(void *)) {
  MALLOC_DEFINE(mp, "testing memory pool");
  kmalloc_init(mp, 1, 10);
  thread_t *threads[THREADS_NUMBER];
  for (int i = 0; i < THREADS_NUMBER; i++)
    threads[i] =
      thread_create("Malloc test thread", threads_function, (void *)mp);
  for (int i = 0; i < THREADS_NUMBER; i++)
    sched_add(threads[i]);
  for (int i = 0; i < THREADS_NUMBER; i++)
    thread_join(threads[i]);
}

static int malloc_threads_one_block(void) {
  malloc_multithreaded(malloc_one_block_at_a_time);
  return KTEST_SUCCESS;
}

static int malloc_threads_many_blocks(void) {
  malloc_multithreaded(malloc_many_blocks_at_a_time);
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
KTEST_ADD(malloc_threads_one_block, malloc_threads_one_block, 0);
KTEST_ADD(malloc_threads_many_blocks, malloc_threads_many_blocks, 0);
/* Reserve some memory for mem_block_t. */
#define RESERVED 1024
KTEST_ADD_RANDINT(malloc_random_size, malloc_random_size, 0,
                  MALLOC_RANDINT_PAGES *PAGESIZE - RESERVED);
