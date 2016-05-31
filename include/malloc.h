#ifndef __SYS_MALLOC_H__
#define __SYS_MALLOC_H__

#include <common.h>
#include <vm_phys.h>

/*
 * This function provides simple dynamic memory allocation that may be used
 * before any memory management has been initialised. This is useful, because
 * sometimes it is needed to dynamically allocate memory for some data
 * structures that are in use even before virtual memory management is ready.
 * This function does it be pretending that the kernel's .bss section ends
 * further than it originally did. In order to allocate N bytes of memory, it
 * pushes the pointer to the end of kernel's image by N bytes. This way such
 * dynamically allocated structures are appended to the memory already occupied
 * by kernel data. The extended end of kernel image is stored in
 * kernel_bss_end, the physical memory manager will take it into account when
 * started.
 *
 * The returned pointer is word-aligned. The block is filled with 0's.
 */
void *kernel_sbrk(size_t size) __attribute__((warn_unused_result));
void *kernel_sbrk_shutdown();

/*
 * General purpose kernel memory allocator.
 */

#define MB_MAGIC     0xC0DECAFE
#define MB_ALIGNMENT sizeof(uint64_t)

TAILQ_HEAD(mb_list, mem_block);
TAILQ_HEAD(mp_arena, mem_arena);

typedef struct mp_arena mp_arena_t;

typedef struct mem_block {
  uint32_t mb_magic; /* if overwritten report a memory corruption error */
  int32_t mb_size;   /* size > 0 => free, size < 0 => alloc'd */
  TAILQ_ENTRY(mem_block) mb_list;
  uint64_t mb_data[0];
} mem_block_t;

typedef struct mem_arena {
  TAILQ_ENTRY(mem_arena) ma_list;
  uint32_t ma_size;                 /* Size of all the blocks inside combined */
  uint16_t ma_flags;
  struct mb_list ma_freeblks;
  uint32_t ma_magic;                /* Detect programmer error. */
  uint64_t ma_data[0];              /* For alignment */
} mem_arena_t;

typedef struct malloc_pool {
  SLIST_ENTRY(malloc_pool) mp_next; /* Next in global chain. */
  uint32_t mp_magic;                /* Detect programmer error. */
  const char *mp_desc;              /* Printable type name. */
  mp_arena_t mp_arena;              /* First managed arena. */
} malloc_pool_t;

/* Defines a local pool of memory for use by a subsystem. */
#define MALLOC_DEFINE(pool, desc) \
    malloc_pool_t pool[1] = {     \
        {{NULL}, MB_MAGIC, desc, {NULL, NULL} }  \
    };

#define MALLOC_DECLARE(pool)      \
    extern malloc_pool_t pool[1]

/* Flags to malloc */
#define M_WAITOK    0x0000 /* ignore for now */
#define M_NOWAIT    0x0001 /* ignore for now */
#define M_ZERO      0x0002 /* clear allocated block */

void kmalloc_init(malloc_pool_t *mp);
void kmalloc_add_arena(malloc_pool_t *mp, vm_addr_t, size_t size);
void *kmalloc(malloc_pool_t *mp, size_t size, uint16_t flags) __attribute__ ((warn_unused_result));
void kfree(malloc_pool_t *mp, void *addr);
void kmalloc_dump(malloc_pool_t *mp);
void kmalloc_test();

#endif /* __SYS_MALLOC_H__ */
