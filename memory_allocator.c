//#include <common.h>
//#include "libkern.h"
#include <stdint.h>
#include "queue.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
//#include "include/memory_allocator.h"

#define MB_MAGIC 0xC0DECAFE

TAILQ_HEAD(mb_list, mem_block);
TAILQ_HEAD(mp_arena, mem_arena);

typedef struct mem_block {
    uint32_t mb_magic;               /* if overwritten report a memory corruption error */
    uint32_t mb_size;                /* size < 0 => free, size > 0 => alloc'd */
    uint16_t mb_flags;
    TAILQ_ENTRY(mem_block) mb_list;
    uint8_t mb_data[0];
} mem_block_t;

#define MB_UNIT sizeof(mem_block_t)

#if 0  // TODO
#define MA_SINGLE 0 /* Single large block allocated within arena. */
#define MA_BLOCKS 1 /* Arena provides space for small blocks. */

/* Allocation larger than MA_THRESHOLD will be handled by allocation of single arena. */
#define MA_THRESHOLD (2 * VM_PAGESIZE)
#endif

typedef struct mem_arena {
    TAILQ_ENTRY(mem_arena) ma_list;
    uint16_t ma_pages; /* Size in pages. */
    struct mb_list ma_freeblks;
    struct mb_list ma_usedblks;
} __attribute__((aligned(MB_UNIT))) mem_arena_t;
//} mem_arena_t;

/* Flags to malloc */
//#define M_WAITOK    0x0000 /* ignore for now */
//#define M_NOWAIT    0x0001 /* ignore for now */
#define M_ZERO      0x0002 /* clear allocated block */

typedef struct malloc_pool {
    SLIST_ENTRY(malloc_pool) mp_next;  /* Next in global chain. */
    uint32_t mp_magic;                 /* Detect programmer error. */
    const char *mp_desc;               /* Printable type name. */
    struct mp_arena mp_arena; /* First managed arena. */
} malloc_pool_t;

/* Defines a local pool of memory for use by a subsystem. */
#define MALLOC_DEFINE(pool, desc)     \
    malloc_pool_t pool[1] = {         \
        { NULL, MB_MAGIC, desc, NULL } \
    };

#define MALLOC_DECLARE(pool) \
    extern malloc_pool_t pool[1]

void malloc_add_arena(malloc_pool_t *mp, void *start, size_t size);
void *malloc2(size_t size, malloc_pool_t *mp, uint16_t flags);
//void *realloc(void *addr, size_t size, malloc_pool_t *mp, uint16_t flags);
void free2(void *addr, malloc_pool_t *mp);



void merge_right(struct mb_list *ma_freeblks, mem_block_t *mb)
{
  mem_block_t *next = TAILQ_NEXT(mb, mb_list);

  if (!next)
    return;

  char *mb_ptr = (char *)mb;
  if (mb_ptr + mb->mb_size + sizeof(mem_block_t) == (char *) next) {
    TAILQ_REMOVE(ma_freeblks, next, mb_list);
    mb->mb_size += next->mb_size + sizeof(mem_block_t);
  }
}

void add_free_memory_block(mem_arena_t* ma, mem_block_t* mb, size_t size)
{
  memset(mb, 0, sizeof(mem_block_t));
  mb->mb_magic = MB_MAGIC;
  mb->mb_size = -(size - sizeof(mem_block_t));
  mb->mb_flags = 0;

  // If it's the first block, we simply add it.
  if (TAILQ_EMPTY(&ma->ma_freeblks)) {
    TAILQ_INSERT_HEAD(&ma->ma_freeblks, mb, mb_list);
    return;
  }

  // It's not the first block, so we insert it in a sorted fashion.
  mem_block_t* current = NULL;
  mem_block_t* best_so_far = NULL;  /* mb can be inserted after this entry. */

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


void malloc_add_arena(malloc_pool_t *mp, void *start, size_t arena_size)
{
  if (arena_size < sizeof(mem_arena_t))
    return;

  memset(start, 0, sizeof(mem_arena_t));
  mem_arena_t *ma = start;

  TAILQ_INSERT_HEAD(&mp->mp_arena, ma, ma_list);
  ma->ma_pages = 0; // TODO

  // Adding the first free block.
  mem_block_t *mb = (mem_block_t*)((char*)mp + sizeof(mem_arena_t));
  size_t block_size = arena_size - sizeof(mem_arena_t);
  add_free_memory_block(ma, mb, block_size);




  


}








int main()
{
  char text[] = "this is a pool for testing purposes";
  MALLOC_DEFINE(test_pool, text);

  char array[3000];
  malloc_add_arena(test_pool, (array) + ((long long)array)%8, 2000);




  return 0;
}