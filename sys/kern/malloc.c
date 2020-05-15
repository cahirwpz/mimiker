#define KL_LOG KL_KMEM
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/mutex.h>
#include <sys/malloc.h>
#include <sys/kmem.h>
#include <sys/pool.h>
#include <sys/errno.h>
#include <sys/kasan.h>
#include <sys/queue.h>

#define MB_MAGIC 0xC0DECAFE
#define MB_ALIGNMENT sizeof(uint64_t)

typedef TAILQ_HEAD(, mem_arena) mem_arena_list_t;
typedef TAILQ_HEAD(, mem_block) mem_block_list_t;

typedef struct kmalloc_pool {
  SLIST_ENTRY(kmalloc_pool) mp_next; /* Next in global chain. */
  uint32_t mp_magic;                 /* Detect programmer error. */
  const char *mp_desc;               /* Printable type name. */
  mem_arena_list_t mp_arena;         /* Queue of managed arenas. */
  mtx_t mp_lock;                     /* Mutex protecting structure */
  size_t mp_used;                    /* Current number of pages (in bytes) */
  size_t mp_maxsize;                 /* Number of pages allowed (in bytes) */
#if KASAN
  quar_t mp_quarantine;
#endif /* !KASAN */
} kmalloc_pool_t;

/*
  TODO:
  - use the mp_next field of kmalloc_pool
*/

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
  mem_block_list_t ma_freeblks;
  uint32_t ma_magic;   /* Detect programmer error. */
  uint64_t ma_data[0]; /* For alignment */
} mem_arena_t;

static inline mem_block_t *mb_next(mem_block_t *block) {
  return (void *)block + abs(block->mb_size) + sizeof(mem_block_t);
}

static void merge_right(mem_block_list_t *ma_freeblks, mem_block_t *mb) {
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
  /* Unpoison and setup the header */
  kasan_mark_valid(mb, sizeof(mem_block_t));
  mb->mb_magic = MB_MAGIC;
  mb->mb_size = total_size - sizeof(mem_block_t);
  /* Poison the data */
  kasan_mark_invalid(mb->mb_data, mb->mb_size, KASAN_CODE_KMALLOC_FREED);

  /* If it's the first block, we simply add it. */
  if (TAILQ_EMPTY(&ma->ma_freeblks)) {
    TAILQ_INSERT_HEAD(&ma->ma_freeblks, mb, mb_list);
    return;
  }

  /* It's not the first block, so we insert it in a sorted fashion. */
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

static void kmalloc_add_arena(kmalloc_pool_t *mp, void *start, size_t size) {
  if (size < sizeof(mem_arena_t))
    return;

  memset((void *)start, 0, sizeof(mem_arena_t));
  mem_arena_t *ma = (void *)start;

  TAILQ_INSERT_HEAD(&mp->mp_arena, ma, ma_list);
  ma->ma_size = size - sizeof(mem_arena_t);
  ma->ma_magic = MB_MAGIC;
  ma->ma_flags = 0;

  TAILQ_INIT(&ma->ma_freeblks);

  /* Adding the first free block that covers all the remaining arena_size. */
  mem_block_t *mb = (mem_block_t *)((char *)ma + sizeof(mem_arena_t));
  size_t block_size = size - sizeof(mem_arena_t);
  add_free_memory_block(ma, mb, block_size);
}

static int kmalloc_add_pages(kmalloc_pool_t *mp, size_t size) {
  assert(mtx_owned(&mp->mp_lock));

  size = roundup(size, PAGESIZE);
  if (mp->mp_used + size > mp->mp_maxsize)
    return ENOMEM;

  void *arena = kmem_alloc(size, M_WAITOK);
  if (arena == NULL)
    return EAGAIN;

  kmalloc_add_arena(mp, arena, size);
  klog("add arena %08x - %08x to '%s' kmem pool", arena, arena + size,
       mp->mp_desc);
  mp->mp_used += size;
  return 0;
}

static mem_block_t *find_entry(mem_block_list_t *mb_list, size_t total_size) {
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
  } else {
    mb->mb_size = -mb->mb_size;
  }

  return mb;
}

void *kmalloc(kmalloc_pool_t *mp, size_t size, unsigned flags) {
  if (size == 0)
    return NULL;

#if KASAN
  /* the alignment is within the redzone */
  size_t redzone_size =
    align(size, MB_ALIGNMENT) - size + KASAN_KMALLOC_REDZONE_SIZE;
#else /* !KASAN */
  /* no redzone, we have to align the size itself */
  size = align(size, MB_ALIGNMENT);
#endif

  SCOPED_MTX_LOCK(&mp->mp_lock);

  while (1) {
    /* Search for the first area in the list that has enough space. */
    mem_arena_t *current = NULL;
    size_t requested_size = size;
#if KASAN
    requested_size += redzone_size;
#endif /* !KASAN */
    TAILQ_FOREACH (current, &mp->mp_arena, ma_list) {
      assert(current->ma_magic == MB_MAGIC);
      mem_block_t *mb = try_allocating_in_area(current, requested_size);
      if (!mb)
        continue;

      /* Create redzone after the buffer */
      kasan_mark(mb->mb_data, size, size + redzone_size,
                 KASAN_CODE_KMALLOC_OVERFLOW);
      if (flags == M_ZERO)
        memset(mb->mb_data, 0, size);
      return mb->mb_data;
    }

    /* Couldn't find any continuous memory with the requested size. */
    if (flags & M_NOWAIT)
      return NULL;

    if (kmalloc_add_pages(mp, size + sizeof(mem_arena_t)))
      panic("memory exhausted in '%s'", mp->mp_desc);
  }
}

static mem_block_t *addr_to_mem_block(void *addr) {
  return (mem_block_t *)((char *)addr - sizeof(mem_block_t));
}

static void _kfree(kmalloc_pool_t *mp, void *addr) {
  assert(mtx_owned(&mp->mp_lock));

  mem_block_t *mb = addr_to_mem_block(addr);
  if (mb->mb_magic != MB_MAGIC || mp->mp_magic != MB_MAGIC || mb->mb_size >= 0)
    panic("Memory corruption detected!");

  mem_arena_t *current = NULL;
  TAILQ_FOREACH (current, &mp->mp_arena, ma_list) {
    char *start = ((char *)current) + sizeof(mem_arena_t);
    if ((char *)addr >= start && (char *)addr < start + current->ma_size)
      add_free_memory_block(current, mb,
                            abs(mb->mb_size) + sizeof(mem_block_t));
  }
}

void kfree(kmalloc_pool_t *mp, void *addr) {
  if (!addr)
    return;
  SCOPED_MTX_LOCK(&mp->mp_lock);

  kasan_mark_invalid(addr, abs(addr_to_mem_block(addr)->mb_size),
                     KASAN_CODE_KMALLOC_FREED);
  kasan_quar_additem(&mp->mp_quarantine, addr);
#ifndef KASAN
  /* Without KASAN, call regular free method */
  _kfree(mp, addr);
#endif /* !KASAN */
}

char *kstrndup(kmalloc_pool_t *mp, const char *s, size_t maxlen) {
  size_t n = strnlen(s, maxlen) + 1;
  char *copy = kmalloc(mp, n, M_ZERO);
  memcpy(copy, s, n);
  return copy;
}

static POOL_DEFINE(P_KMEM, "kmem", sizeof(kmalloc_pool_t));

void kmalloc_bootstrap(void) {
  INVOKE_CTORS(kmalloc_ctor_table);
}

kmalloc_pool_t *kmalloc_create(const char *desc, size_t maxsize) {
  assert(is_aligned(maxsize, PAGESIZE));

  kmalloc_pool_t *mp = pool_alloc(P_KMEM, M_ZERO);
  mp->mp_desc = desc;
  mp->mp_used = 0;
  mp->mp_maxsize = maxsize;
  mp->mp_magic = MB_MAGIC;
  TAILQ_INIT(&mp->mp_arena);
  mtx_init(&mp->mp_lock, 0);
  kasan_quar_init(&mp->mp_quarantine, mp, (quar_free_t)_kfree);
  klog("initialized '%s' kmem at %p ", mp->mp_desc, mp);
  return mp;
}

int kmalloc_reserve(kmalloc_pool_t *mp, size_t size) {
  SCOPED_MTX_LOCK(&mp->mp_lock);
  return kmalloc_add_pages(mp, size);
}

void kmalloc_dump(kmalloc_pool_t *mp) {
  klog("pool at %p:", mp);

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
void kmalloc_destroy(kmalloc_pool_t *mp) {
  WITH_MTX_LOCK (&mp->mp_lock)
    /* Lock needed as the quarantine may call _kfree! */
    kasan_quar_releaseall(&mp->mp_quarantine);
  pool_free(P_KMEM, mp);
}

KMALLOC_DEFINE(M_TEMP, "temporaries pool", PAGESIZE * 25);
KMALLOC_DEFINE(M_STR, "strings", PAGESIZE * 4);
