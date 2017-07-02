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

#define ALLOCATIONS_PER_THREAD 1000
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
  kmem_pool_t *mp = KMEM_POOL("test", 1, 10);
  kmem_init(mp);
  thread_t *threads[THREADS_NUMBER];
  for (int i = 0; i < THREADS_NUMBER; i++)
    threads[i] =
      thread_create("Malloc test thread", threads_function, (void *)mp);
  for (int i = 0; i < THREADS_NUMBER; i++)
    sched_add(threads[i]);
  for (int i = 0; i < THREADS_NUMBER; i++)
    thread_join(threads[i]);
}

static int malloc_threads_private_block(void) {
  malloc_multithreaded(malloc_one_block_at_a_time);
  return KTEST_SUCCESS;
}

static int malloc_threads_many_private_blocks(void) {
  malloc_multithreaded(malloc_many_blocks_at_a_time);
  return KTEST_SUCCESS;
}

#define PTRS_ARRAY_SIZE 100
typedef struct rsb_test_args {
  mtx_t lock;
  kmem_pool_t *mem_pool;
  void *ptrs[PTRS_ARRAY_SIZE];
} rsb_test_args_t;

static void malloc_random_shared_blocks(void *arg) {
  rsb_test_args_t *args = (rsb_test_args_t *)arg;
  unsigned seed = (unsigned)thread_self();
  for (int i = 0; i < ALLOCATIONS_PER_THREAD;) {
    unsigned idx = ((unsigned)rand_r(&seed)) % PTRS_ARRAY_SIZE;
    WITH_MTX_LOCK (&args->lock) {
      if (args->ptrs[idx]) {
        kfree(args->mem_pool, args->ptrs[idx]);
        args->ptrs[idx] = NULL;
      } else {
        void *ptr = kmalloc(args->mem_pool, 1, 0);
        assert(ptr != NULL);
        args->ptrs[idx] = ptr;
        i++;
      }
    }
  }
}

static int malloc_threads_random_shared_blocks(void) {
  kmem_pool_t *mp = KMEM_POOL("test", 1, 10);
  kmem_init(mp);
  rsb_test_args_t args;
  memset(args.ptrs, 0, sizeof(args.ptrs));
  args.mem_pool = mp;
  mtx_init(&args.lock, MTX_DEF);
  thread_t *threads[THREADS_NUMBER];
  for (int i = 0; i < THREADS_NUMBER; i++)
    threads[i] =
      thread_create("Malloc test thread", malloc_random_shared_blocks, &args);
  for (int i = 0; i < THREADS_NUMBER; i++)
    sched_add(threads[i]);
  for (int i = 0; i < THREADS_NUMBER; i++)
    thread_join(threads[i]);
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
KTEST_ADD(malloc_threads_private_block, malloc_threads_private_block,
          KTEST_FLAG_BROKEN);
KTEST_ADD(malloc_threads_many_private_blocks,
          malloc_threads_many_private_blocks, KTEST_FLAG_BROKEN);
KTEST_ADD(malloc_threads_random_shared_blocks,
          malloc_threads_random_shared_blocks, KTEST_FLAG_BROKEN);
/* Reserve some memory for mem_block_t. */
#define RESERVED 1024
KTEST_ADD_RANDINT(malloc_random_size, (int (*)(void))malloc_random_size, 0,
                  MALLOC_RANDINT_PAGES *PAGESIZE - RESERVED);
