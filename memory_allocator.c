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

static void merge_right(struct mb_list *ma_freeblks, mem_block_t *mb) {
  mem_block_t *next = TAILQ_NEXT(mb, mb_list);

  if (!next)
    return;

  char *mb_ptr = (char *)mb;
  if (mb_ptr + mb->mb_size + sizeof(mem_block_t) == (char *) next) {
    TAILQ_REMOVE(ma_freeblks, next, mb_list);
    mb->mb_size += next->mb_size + sizeof(mem_block_t);
  }
}

static void add_free_memory_block(mem_arena_t *ma, mem_block_t *mb,
                                  size_t total_size) {
  memset(mb, 0, sizeof(mem_block_t));
  mb->mb_magic = MAGIC;
  mb->mb_size = total_size - sizeof(mem_block_t);
  mb->mb_flags = 0;

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
    if (current->mb_size >= total_size)
      return current;
  }
  return NULL;
}

static mem_block_t *try_allocating_in_area(mem_arena_t *ma,
    size_t requested_size, uint16_t flags) {
  mem_block_t *mb = find_entry(&ma->ma_freeblks,
                               requested_size + sizeof(mem_block_t));

  if (!mb) /* No entry has enough space. */
    return NULL;

  TAILQ_REMOVE(&ma->ma_freeblks, mb, mb_list);
  size_t total_size_left = mb->mb_size - requested_size;
  if (total_size_left > sizeof(mem_block_t)) {
    mb->mb_size = requested_size;
    mem_block_t *new_mb = (mem_block_t *)((char *)mb + requested_size + sizeof(
                                            mem_block_t));
    add_free_memory_block(ma, new_mb, total_size_left);
  }

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
    mem_block_t *mb = try_allocating_in_area(current, size_aligned, flags);
    if (mb) {
      add_used_memory_block(current, mb);

      if (flags == M_ZERO)
        memset(mb->mb_data, 0, size);

      return mb->mb_data;
    }
  }

  return NULL;
}

void deallocate(void *addr, malloc_pool_t *mp) {
  mem_block_t *mb = (mem_block_t *)(((char *)addr) - sizeof(mem_block_t));

  if (mb->mb_magic != MAGIC || mp->mp_magic != MAGIC)
    kprintf("Memory corruption detected!\n");

  mem_arena_t *current = NULL;
  TAILQ_FOREACH(current, &mp->mp_arena, ma_list) {
    char *start = ((char *)current) + sizeof(mem_arena_t);
    if ((char *)addr >= start && (char *)addr < start + current->ma_size) {
      remove_used_memory_block(current, mb);
      add_free_memory_block(current, mb, mb->mb_size + sizeof(mem_block_t));
    }
  }
}

void print_free_blocks(malloc_pool_t *mp) {
  mem_arena_t *current_arena = NULL;
  TAILQ_FOREACH(current_arena, &mp->mp_arena, ma_list) {
    kprintf("Printing free blocks:\n");
    mem_block_t *current_block = NULL;
    TAILQ_FOREACH(current_block, &current_arena->ma_freeblks, mb_list) {
      kprintf("%zu\n", current_block->mb_size);
    }

    kprintf("Printing used blocks:\n");
    current_block = NULL;
    TAILQ_FOREACH(current_block, &current_arena->ma_usedblks, mb_list) {
      kprintf("%zu\n", current_block->mb_size);
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

  print_free_blocks(test_pool);

  void *ptr1 = allocate(15, test_pool, 0);
  kprintf("%p\n", ptr1);
  void *ptr2 = allocate(15, test_pool, 0);
  kprintf("%p\n", ptr2);
  void *ptr3 = allocate(15, test_pool, 0);
  kprintf("%p\n", ptr3);
  void *ptr4 = allocate(1000, test_pool, 0);
  kprintf("%p\n", ptr4);
  void *ptr5 = allocate(1000, test_pool, 0);
  kprintf("%p\n", ptr5);
  deallocate(ptr3, test_pool);
  deallocate(ptr1, test_pool);
  deallocate(ptr2, test_pool);
  deallocate(ptr4, test_pool);
  void *ptr6 = allocate(1000, test_pool, 0);
  kprintf("%p\n", ptr6);
  deallocate(ptr6, test_pool);

  print_free_blocks(test_pool);
}