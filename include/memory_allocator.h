#ifndef __MEMORY_ALLOCATOR_H__
#define __MEMORY_ALLOCATOR_H__

#define MAGIC 0xC0DECAFE
#define ALIGNMENT 8

TAILQ_HEAD(mb_list, mem_block);
TAILQ_HEAD(mp_arena, mem_arena);

typedef struct mem_block {
  uint32_t mb_magic; /* if overwritten report a memory corruption error */
  uint32_t mb_size;
  uint16_t mb_flags;
  TAILQ_ENTRY(mem_block) mb_list;
  uint64_t mb_data[0];
} mem_block_t;

typedef struct mem_arena {
  TAILQ_ENTRY(mem_arena) ma_list;
  uint32_t ma_size; /* Size of all the blocks inside combined */
  struct mb_list ma_freeblks;
  struct mb_list ma_usedblks;
  uint64_t ma_data[0];
} mem_arena_t;

typedef struct malloc_pool {
  SLIST_ENTRY(malloc_pool) mp_next;  /* Next in global chain. */
  uint32_t mp_magic;                 /* Detect programmer error. */
  const char *mp_desc;               /* Printable type name. */
  struct mp_arena mp_arena; /* First managed arena. */
} malloc_pool_t;


/* Defines a local pool of memory for use by a subsystem. */
#define MALLOC_DEFINE(pool, desc)     \
    malloc_pool_t pool[1] = {         \
        {{NULL}, MAGIC, desc} \
    };

#define MALLOC_DECLARE(pool) \
    extern malloc_pool_t pool[1]


/* Flags to malloc */
#if 0  // TODO
#define M_WAITOK    0x0000 /* ignore for now */
#define M_NOWAIT    0x0001 /* ignore for now */
#endif
#define M_ZERO      0x0002 /* clear allocated block */


void malloc_add_arena(malloc_pool_t *mp, void *start, size_t size);
void *allocate(size_t size, malloc_pool_t *mp, uint16_t flags);
//void *realloc(void *addr, size_t size, malloc_pool_t *mp, uint16_t flags);
void deallocate(void *addr, malloc_pool_t *mp);
void print_free_blocks(malloc_pool_t *mp);
void test_memory_allocator();



#endif
