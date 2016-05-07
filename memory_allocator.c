//#include <common.h>
//#include "libkern.h"
#include <stdint.h>
#include "queue.h"
#include <stddef.h>
#include <stdio.h>
//#include "include/memory_allocator.h"

#define MB_MAGIC 0xC0DECAFE

TAILQ_HEAD(mb_list, mem_block);
TAILQ_HEAD(mp_arena, mem_arena);

typedef struct mem_block {
    uint32_t mb_magic;               /* if overwritten report a memory corruption error */
    uint32_t mb_size;                /* size < 0 => free, size > 0 => alloc'd */
    struct mb_list mb_list;
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
    uint16_t ma_flags;
    struct mb_list ma_freeblks;
    struct mb_list ma_usedblks;
//} __attribute__((aligned(MB_UNIT))) mem_arena_t;
} mem_arena_t;

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




void malloc_add_arena(malloc_pool_t *mp, void *start, size_t size)
{
  mem_arena_t *ma = start;

  TAILQ_INSERT_HEAD(&mp->mp_arena, ma, ma_list);
}








int main()
{
  char text[] = "this is a pool for testing purposes";
  MALLOC_DEFINE(test_pool, text);

  char array[3000];
  malloc_add_arena(test_pool, (array) + ((long long)array)%8, 2000);




  return 0;
}