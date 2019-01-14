#define KL_LOG KL_KMEM
#include <klog.h>
#include <stdc.h>
#include <mutex.h>
#include <malloc.h>
#include <vm_map.h>
#include <pool.h>
#include <queue.h>

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

/*
  TODO:
  - use the mp_next field of kmem_pool
*/

TAILQ_HEAD(mb_list, mem_block);

typedef struct mem_block {
  uint32_t mb_magic; /* if overwritten report a memory corruption error */
  int32_t mb_size;   /* size > 0 => free, size < 0 => alloc'd */
  TAILQ_ENTRY(mem_block) mb_list;
  uint64_t mb_data[0];
} mem_block_t;

typedef struct mem_arena {
  TAILQ_ENTRY(mem_arena) ma_list;
  uint32_t ma_size; /* Size of all the blocks inside combined */
  uint16_t ma_flags;
  struct mb_list ma_freeblks;
  uint32_t ma_magic;   /* Detect programmer error. */
  uint64_t ma_data[0]; /* For alignment */
} mem_arena_t;

static inline mem_block_t *mb_next(mem_block_t *block) {
  return (void *)block + abs(block->mb_size) + sizeof(mem_block_t);
}

static void merge_right(struct mb_list *ma_freeblks, mem_block_t *mb) {
  mem_block_t *next = TAILQ_NEXT(mb, mb_list);

  if (!next)
    return;

  char *mb_ptr = (char *)mb;
  if (mb_ptr + mb->mb_size + sizeof(mem_block_t) == (char *)next) {
    TAILQ_REMOVE(ma_freeblks, next, mb_list);
    mb->mb_size = mb->mb_size + next->mb_size + sizeof(mem_block_t);
  }
}

static void add_free_memory_block(mem_arena_t *ma, mem_block_t *mb,
                                  size_t total_size) {
  memset(mb, 0, sizeof(mem_block_t));
  mb->mb_magic = MB_MAGIC;
  mb->mb_size = total_size - sizeof(mem_block_t);

  // If it's the first block, we simply add it.
  if (TAILQ_EMPTY(&ma->ma_freeblks)) {
    TAILQ_INSERT_HEAD(&ma->ma_freeblks, mb, mb_list);
    return;
  }

  // It's not the first block, so we insert it in a sorted fashion.
  mem_block_t *current = NULL;
  mem_block_t *best_so_far = NULL; /* mb can be inserted after this entry. */

  TAILQ_FOREACH (current, &ma->ma_freeblks, mb_list) {
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

static void kmalloc_add_arena(kmem_pool_t *mp, vaddr_t start,
                              size_t arena_size) {
  if (arena_size < sizeof(mem_arena_t))
    return;

  memset((void *)start, 0, sizeof(mem_arena_t));
  mem_arena_t *ma = (void *)start;

  TAILQ_INSERT_HEAD(&mp->mp_arena, ma, ma_list);
  ma->ma_size = arena_size - sizeof(mem_arena_t);
  ma->ma_magic = MB_MAGIC;
  ma->ma_flags = 0;

  TAILQ_INIT(&ma->ma_freeblks);

  // Adding the first free block that covers all the remaining arena_size.
  mem_block_t *mb = (mem_block_t *)((char *)ma + sizeof(mem_arena_t));
  size_t block_size = arena_size - sizeof(mem_arena_t);
  add_free_memory_block(ma, mb, block_size);
}

static void kmalloc_add_pages(kmem_pool_t *mp, unsigned pages) {
  vm_segment_t *seg;
  int error = vm_map_alloc_segment(get_kernel_vm_map(), 0, pages * PAGESIZE,
                                   VM_PROT_READ | VM_PROT_WRITE, VM_ANON, &seg);
  if (error)
    panic("failed to alloc pages for '%s'", mp->mp_desc);
  vaddr_t start, end;
  vm_segment_range(seg, &start, &end);
  kmalloc_add_arena(mp, start, end - start);
}

static mem_block_t *find_entry(struct mb_list *mb_list, size_t total_size) {
  mem_block_t *current = NULL;
  TAILQ_FOREACH (current, mb_list, mb_list) {
    assert(current->mb_magic == MB_MAGIC);
    if (current->mb_size >= (ssize_t)total_size)
      return current;
  }
  return NULL;
}

static mem_block_t *try_allocating_in_area(mem_arena_t *ma,
                                           size_t requested_size) {
  mem_block_t *mb =
    find_entry(&ma->ma_freeblks, requested_size + sizeof(mem_block_t));

  if (!mb) /* No entry has enough space. */
    return NULL;

  TAILQ_REMOVE(&ma->ma_freeblks, mb, mb_list);
  size_t total_size_left = mb->mb_size - requested_size;
  if (total_size_left > sizeof(mem_block_t)) {
    mb->mb_size = -requested_size;
    mem_block_t *new_mb =
      (mem_block_t *)((char *)mb + requested_size + sizeof(mem_block_t));
    add_free_memory_block(ma, new_mb, total_size_left);
  } else
    mb->mb_size = -mb->mb_size;

  return mb;
}

void *kmalloc(kmem_pool_t *mp, size_t size, unsigned flags) {
  size_t size_aligned = align(size, MB_ALIGNMENT);
  if (size_aligned == 0)
    return NULL;

  SCOPED_MTX_LOCK(&mp->mp_lock);

  /* Search for the first area in the list that has enough space. */
  mem_arena_t *current = NULL;
  TAILQ_FOREACH (current, &mp->mp_arena, ma_list) {
    assert(current->ma_magic == MB_MAGIC);

    mem_block_t *mb = try_allocating_in_area(current, size_aligned);

    if (mb) {
      if (flags == M_ZERO)
        memset(mb->mb_data, 0, size);
      return mb->mb_data;
    }
  }

  /* Couldn't find any continuous memory with the requested size. */
  if (flags & M_NOWAIT)
    return NULL;

  size_t pages = roundup(size_aligned, PAGESIZE) / PAGESIZE;

  if (mp->mp_pages_used + pages <= mp->mp_pages_max) {
    kmalloc_add_pages(mp, pages);
    mp->mp_pages_used += pages;
    return kmalloc(mp, size, flags);
  }

  panic("memory exhausted in '%s'", mp->mp_desc);
}

void kfree(kmem_pool_t *mp, void *addr) {
  mem_block_t *mb = (mem_block_t *)(((char *)addr) - sizeof(mem_block_t));

  if (mb->mb_magic != MB_MAGIC || mp->mp_magic != MB_MAGIC || mb->mb_size >= 0)
    panic("Memory corruption detected!");

  SCOPED_MTX_LOCK(&mp->mp_lock);

  mem_arena_t *current = NULL;
  TAILQ_FOREACH (current, &mp->mp_arena, ma_list) {
    char *start = ((char *)current) + sizeof(mem_arena_t);
    if ((char *)addr >= start && (char *)addr < start + current->ma_size)
      add_free_memory_block(current, mb,
                            abs(mb->mb_size) + sizeof(mem_block_t));
  }
}

char *kstrndup(kmem_pool_t *mp, const char *s, size_t maxlen) {
  size_t n = strnlen(s, maxlen) + 1;
  char *copy = kmalloc(mp, n, M_ZERO);
  memcpy(copy, s, n);
  return copy;
}

void kmem_bootstrap(void) {
  INVOKE_CTORS(kmem_ctor_table);
}

static void kmem_init(kmem_pool_t *mp) {
  mp->mp_magic = MB_MAGIC;
  TAILQ_INIT(&mp->mp_arena);
  mtx_init(&mp->mp_lock, LK_RECURSE);
  kmalloc_add_pages(mp, mp->mp_pages_used);
  klog("initialized '%s' kmem at %p ", mp->mp_desc, mp);
}

static POOL_DEFINE(P_KMEM, "kmem", sizeof(kmem_pool_t));

kmem_pool_t *kmem_create(const char *desc, size_t pg_used, size_t pg_max) {
  kmem_pool_t *mp = pool_alloc(P_KMEM, PF_ZERO);
  mp->mp_desc = desc;
  mp->mp_pages_used = pg_used;
  mp->mp_pages_max = pg_max;
  kmem_init(mp);
  return mp;
}

void kmem_dump(kmem_pool_t *mp) {
  klog("pool at %p:", mp);

  SCOPED_MTX_LOCK(&mp->mp_lock);

  mem_arena_t *arena = NULL;
  TAILQ_FOREACH (arena, &mp->mp_arena, ma_list) {
    mem_block_t *block = (void *)arena->ma_data;
    mem_block_t *end = (void *)arena->ma_data + arena->ma_size;

    klog("> malloc_arena %p - %p:", block, end);

    while (block < end) {
      assert(block->mb_magic == MB_MAGIC);
      klog("   %c %p %d", (block->mb_size > 0) ? 'F' : 'U', block,
           (unsigned)abs(block->mb_size));
      block = mb_next(block);
    }
  }
}

/* TODO: missing implementation */
void kmem_destroy(kmem_pool_t *mp) {
  pool_free(P_KMEM, mp);
}

MALLOC_DEFINE(M_TEMP, "temporaries pool", 2, 4);
MALLOC_DEFINE(M_STR, "strings", 2, 4);
