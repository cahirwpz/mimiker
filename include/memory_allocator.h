#ifndef __MEMORY_ALLOCATOR_H__
#define __MEMORY_ALLOCATOR_H__

typedef struct malloc_pool malloc_pool_t;

/* Defines a local pool of memory for use by a subsystem. */
#define MALLOC_DEFINE(pool, desc)     \
    malloc_pool_t pool[1] = {         \
        {{NULL}, MAGIC, desc} \
    };

#define MALLOC_DECLARE(pool) \
    extern malloc_pool_t pool[1]

/* Flags to malloc */
#define M_WAITOK    0x0000 /* ignore for now */
#define M_NOWAIT    0x0001 /* ignore for now */
#define M_ZERO      0x0002 /* clear allocated block */

void malloc_add_arena(malloc_pool_t *mp, void *start, size_t size);
void *allocate(size_t size, malloc_pool_t *mp, uint16_t flags);
//void *realloc(void *addr, size_t size, malloc_pool_t *mp, uint16_t flags);
void deallocate(void *addr, malloc_pool_t *mp);
void print_free_blocks(malloc_pool_t *mp);
void test_memory_allocator();

#endif /* __MEMORY_ALLOCATOR_H__ */
