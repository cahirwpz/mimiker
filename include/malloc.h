#ifndef _MALLOC_H_
#define _MALLOC_H_

#include <common.h>
#include <physmem.h>
#include <mutex.h>
#include <linker_set.h>

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
/* If you still see this in late 2017 please get rid of it
 * void kernel_brk(void *addr);
 */
void *kernel_sbrk(size_t size) __attribute__((warn_unused_result));

/*
 * General purpose kernel memory allocator.
 */

#define MB_MAGIC 0xC0DECAFE
#define MB_ALIGNMENT sizeof(uint64_t)

typedef TAILQ_HEAD(, mem_arena) mem_arena_list_t;

typedef struct kmem_pool {
  SLIST_ENTRY(kmem_pool) mp_next; /* Next in global chain. */
  uint32_t mp_magic;              /* Detect programmer error. */
  const char *mp_desc;            /* Printable type name. */
  mem_arena_list_t mp_arena;      /* Queue of managed arenas. */
  mtx_t mp_lock;                  /* Mutex protecting structure */
  unsigned mp_pages_used;         /* Current number of pages */
  unsigned mp_pages_max;          /* Number of pages allowed */
} kmem_pool_t;

#define KMEM_POOL(desc, npages, maxpages)                                      \
  (kmem_pool_t[1]) {                                                           \
    {                                                                          \
      .mp_magic = MB_MAGIC, .mp_desc = (desc), .mp_pages_used = (npages),      \
      .mp_pages_max = (maxpages)                                               \
    }                                                                          \
  }

/* Defines a local pool of memory for use by a subsystem. */
#define MALLOC_DEFINE(pool, desc, npages, maxpages)                            \
  kmem_pool_t *pool = KMEM_POOL((desc), (npages), (maxpages));                 \
  SET_ENTRY(kmem_pool_table, pool)

#define MALLOC_DECLARE(pool) extern kmem_pool_t *pool;

/* Flags to malloc */
#define M_WAITOK 0x0000 /* always returns memory block, but can sleep */
#define M_NOWAIT 0x0001 /* may return NULL, but cannot sleep */
#define M_ZERO 0x0002   /* clear allocated block */

void kmem_bootstrap(void);
void kmem_init(kmem_pool_t *mp);
void kmem_dump(kmem_pool_t *mp);
void kmem_destroy(kmem_pool_t *mp);

void *kmalloc(kmem_pool_t *mp, size_t size, unsigned flags) __warn_unused;
void kfree(kmem_pool_t *mp, void *addr);
char *kstrndup(kmem_pool_t *mp, const char *s, size_t maxlen);

MALLOC_DECLARE(M_TEMP);

#endif /* _MALLOC_H_ */
