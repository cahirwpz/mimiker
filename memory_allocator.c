#include <common.h>
#include <stdint.h>
#include <stddef.h>
#include "queue.h"
#include "libkern.h"
#include "include/memory_allocator.h"

/*
  TODO:
  - use the mp_next field of malloc_pool
*/

#define MAGIC 0xC0DECAFE
#define ALIGNMENT 8

TAILQ_HEAD(mb_list, mem_block);
TAILQ_HEAD(mp_arena, mem_arena);

static inline int32_t abs(int32_t x)
{
  if (x < 0)
    return -x;
  return x;
}

typedef struct mem_block {
  uint32_t mb_magic; /* if overwritten report a memory corruption error */
  int32_t mb_size; /* size > 0 => free, size < 0 => alloc'd */
  TAILQ_ENTRY(mem_block) mb_list;
  uint64_t mb_data[0];
} mem_block_t;

typedef struct mem_arena {
  TAILQ_ENTRY(mem_arena) ma_list;
  uint32_t ma_size; /* Size of all the blocks inside combined */
  uint16_t ma_flags;
  struct mb_list ma_freeblks;
  struct mb_list ma_usedblks;
  uint32_t ma_magic; /* Detect programmer error. */
  uint64_t ma_data[0]; /* For alignment */
} mem_arena_t;

typedef struct malloc_pool {
  SLIST_ENTRY(malloc_pool) mp_next;  /* Next in global chain. */
  uint32_t mp_magic;                 /* Detect programmer error. */
  const char *mp_desc;               /* Printable type name. */
  struct mp_arena mp_arena; /* First managed arena. */
} malloc_pool_t;


static void merge_right(struct mb_list *ma_freeblks, mem_block_t *mb) {
  mem_block_t *next = TAILQ_NEXT(mb, mb_list);

  if (!next)
    return;

  char *mb_ptr = (char *)mb;
  if (mb_ptr + mb->mb_size + sizeof(mem_block_t) == (char *) next) {
    TAILQ_REMOVE(ma_freeblks, next, mb_list);
    mb->mb_size = mb->mb_size + next->mb_size + sizeof(mem_block_t);
  }
}

static void add_free_memory_block(mem_arena_t *ma, mem_block_t *mb,
                                  size_t total_size) {
  memset(mb, 0, sizeof(mem_block_t));
  mb->mb_magic = MAGIC;
  mb->mb_size = total_size - sizeof(mem_block_t);

  // If it's the first block, we simply add it.
  if (TAILQ_EMPTY(&ma->ma_freeblks)) {
    TAILQ_INSERT_HEAD(&ma->ma_freeblks, mb, mb_list);
    return;
  }

  // It's not the first block, so we insert it in a sorted fashion.
  mem_block_t *current = NULL;
  mem_block_t *best_so_far = NULL;  /* mb can be inserted after this entry. */

  TAILQ_FOREACH(current, &ma->ma_freeblks, mb_list) {
    if (current < mb)
      best_so_far = current;
  }

  if (!best_so_far) {
    TAILQ_INSERT_HEAD(&ma->ma_freeblks, mb, mb_list);
    merge_right(&ma->ma_freeblks, mb);
  } else {
    TAILQ_INSERT_AFTER(&ma->ma_freeblks, best_so_far, mb, mb_list);
    merge_right(&ma->ma_freeblks, mb);
    merge_right(&ma->ma_freeblks, best_so_far);
  }
}

static void add_used_memory_block(mem_arena_t *ma, mem_block_t *mb) {
  TAILQ_INSERT_HEAD(&ma->ma_usedblks, mb, mb_list);
}

static void remove_used_memory_block(mem_arena_t *ma, mem_block_t *mb) {
  TAILQ_REMOVE(&ma->ma_usedblks, mb, mb_list);
}

void malloc_add_arena(malloc_pool_t *mp, void *start, size_t arena_size) {
  if (arena_size < sizeof(mem_arena_t))
    return;

  memset(start, 0, sizeof(mem_arena_t));
  mem_arena_t *ma = start;

  TAILQ_INSERT_HEAD(&mp->mp_arena, ma, ma_list);
  ma->ma_size = arena_size - sizeof(mem_arena_t);
  ma->ma_magic = MAGIC;
  ma->ma_flags = 0;

  TAILQ_INIT(&ma->ma_freeblks);
  TAILQ_INIT(&ma->ma_usedblks);

  // Adding the first free block that covers all the remaining arena_size.
  mem_block_t *mb = (mem_block_t *)((char *)ma + sizeof(mem_arena_t));
  size_t block_size = arena_size - sizeof(mem_arena_t);
  add_free_memory_block(ma, mb, block_size);
}

static mem_block_t *find_entry(struct mb_list *mb_list, size_t total_size) {
  mem_block_t *current = NULL;
  TAILQ_FOREACH(current, mb_list, mb_list) {
    if (current->mb_magic != MAGIC)
      panic("Memory corruption detected!");

    if (current->mb_size >= total_size)
      return current;
  }
  return NULL;
}

static mem_block_t *try_allocating_in_area(mem_arena_t *ma,
    size_t requested_size) {
  mem_block_t *mb = find_entry(&ma->ma_freeblks,
                               requested_size + sizeof(mem_block_t));

  if (!mb) /* No entry has enough space. */
    return NULL;

  TAILQ_REMOVE(&ma->ma_freeblks, mb, mb_list);
  size_t total_size_left = mb->mb_size - requested_size;
  if (total_size_left > sizeof(mem_block_t)) {
    mb->mb_size = -requested_size;
    mem_block_t *new_mb = (mem_block_t *)((char *)mb + requested_size + sizeof(
                                            mem_block_t));
    add_free_memory_block(ma, new_mb, total_size_left);
  }
  else
    mb->mb_size = -mb->mb_size;

  return mb;
}

void *allocate(size_t size, malloc_pool_t *mp, uint16_t flags) {
  size_t size_aligned;
  if (size % ALIGNMENT == 0)
    size_aligned = size;
  else
    size_aligned = size + ALIGNMENT - size % ALIGNMENT;

  /* Search for the first area in the list that has enough space. */
  mem_arena_t *current = NULL;
  TAILQ_FOREACH(current, &mp->mp_arena, ma_list) {
    if (current->ma_magic != MAGIC)
      panic("Memory corruption detected!");

    mem_block_t *mb = try_allocating_in_area(current, size_aligned);

    if (mb) {
      add_used_memory_block(current, mb);

      if (flags == M_ZERO)
        memset(mb->mb_data, 0, size);

      return mb->mb_data;
    }
  }

  /* Couldn't find any continuous memory with the requested size. */
  return NULL;
}

void deallocate(void *addr, malloc_pool_t *mp) {
  mem_block_t *mb = (mem_block_t *)(((char *)addr) - sizeof(mem_block_t));

  if (mb->mb_magic != MAGIC || mp->mp_magic != MAGIC || mb->mb_size >= 0)
    panic("Memory corruption detected!");

  mem_arena_t *current = NULL;
  TAILQ_FOREACH(current, &mp->mp_arena, ma_list) {
    char *start = ((char *)current) + sizeof(mem_arena_t);
    if ((char *)addr >= start && (char *)addr < start + current->ma_size) {
      remove_used_memory_block(current, mb);
      add_free_memory_block(current, mb, abs(mb->mb_size) + sizeof(mem_block_t));
    }
  }
}

void print_free_blocks(malloc_pool_t *mp) {
  mem_arena_t *current_arena = NULL;
  TAILQ_FOREACH(current_arena, &mp->mp_arena, ma_list) {
    kprintf("Printing free blocks:\n");
    mem_block_t *current_block = NULL;
    TAILQ_FOREACH(current_block, &current_arena->ma_freeblks, mb_list) {
      kprintf("%d\n", current_block->mb_size);
    }

    kprintf("Printing used blocks:\n");
    current_block = NULL;
    TAILQ_FOREACH(current_block, &current_arena->ma_usedblks, mb_list) {
      kprintf("%d\n", current_block->mb_size);
    }
  }
  kprintf("\n");
}

void test_memory_allocator() {
  char text[] = "this is a pool for testing purposes";
  MALLOC_DEFINE(test_pool, text);
  TAILQ_INIT(&test_pool->mp_arena);


  char array[2100];
  malloc_add_arena(test_pool, (array) + ((size_t)array) % 8, 2050);


  void *ptr1 = allocate(15, test_pool, 0);
  if (ptr1 == NULL)
    panic("Test failed");

  void *ptr2 = allocate(15, test_pool, 0);
  if(ptr2 == NULL || ptr2 <= ptr1)
    panic("Test failed");

  void *ptr3 = allocate(15, test_pool, 0);
  if (ptr3 == NULL || ptr3 <= ptr2)
    panic("Test failed");

  void *ptr4 = allocate(1000, test_pool, 0);
  if (ptr4 == NULL || ptr4 <= ptr3)
    panic("Test failed");
  void *ptr5 = allocate(1000, test_pool, 0);
  if (ptr5 != NULL)
    panic("Test failed");

  deallocate(ptr3, test_pool);
  deallocate(ptr1, test_pool);
  deallocate(ptr2, test_pool);
  deallocate(ptr4, test_pool);

  void *ptr6 = allocate(1800, test_pool, 0);
  if (ptr6 == NULL)
    panic("Test failed");

  deallocate(ptr6, test_pool);
}
