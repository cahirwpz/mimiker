//#include <common.h>
//#include "libkern.h"
#include <stdint.h>
#include "queue.h"
#include <stddef.h>
#include <stdio.h>
//#include "include/memory_allocator.h"

#define MB_MAGIC 0xC0DECAFE

typedef struct mem_block {
    uint32_t mb_magic;               /* if overwritten report a memory corruption error */
    uint32_t mb_size;                /* size < 0 => free, size > 0 => alloc'd */
    TAILQ_HEAD(, mem_block) mb_list;
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
    TAILQ_HEAD(, mem_block) ma_freeblks;
    TAILQ_HEAD(, mem_block) ma_usedblks;
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
    TAILQ_HEAD(, mem_arena) *mp_arena; /* First managed arena. */
} malloc_pool_t;

/* Defines a local pool of memory for use by a subsystem. */
#define MALLOC_DEFINE(pool, desc)     \
    malloc_pool_t pool[1] = {         \
        { NULL, M_MAGIC, desc, NULL } \
    };

#define MALLOC_DECLARE(pool) \
    extern malloc_pool_t pool[1]

void malloc_add_arena(malloc_pool_t *mp, void *start, void *end);
void *malloc2(size_t size, malloc_pool_t *mp, uint16_t flags);
//void *realloc(void *addr, size_t size, malloc_pool_t *mp, uint16_t flags);
void free2(void *addr, malloc_pool_t *mp);




int main()
{

  return 0;
}