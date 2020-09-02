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

typedef TAILQ_HEAD(, block) block_list_t;

static TAILQ_HEAD(, arena) arena_list; /* Queue of managed arenas. */
static mtx_t arena_lock;               /* Mutex protecting arena list. */

#if KASAN
static quar_t block_quarantine;
#endif /* !KASAN */

typedef struct block {
  uint32_t mb_magic; /* if overwritten report a memory corruption error */
  int32_t mb_size;   /* size > 0 => free, size < 0 => alloc'd */
  TAILQ_ENTRY(block) mb_list;
  uint64_t mb_data[0];
} block_t;

typedef struct arena {
  TAILQ_ENTRY(arena) ma_list;
  uint32_t ma_size; /* Size of all the blocks inside combined */
  block_list_t ma_freeblks;
  uint32_t ma_magic;   /* Detect programmer error. */
  uint64_t ma_data[0]; /* For alignment */
} arena_t;

static inline block_t *mb_next(block_t *block) {
  return (void *)block + abs(block->mb_size) + sizeof(block_t);
}

static void merge_right(block_list_t *ma_freeblks, block_t *mb) {
  block_t *next = TAILQ_NEXT(mb, mb_list);

  if (!next)
    return;

  char *mb_ptr = (char *)mb;
  if (mb_ptr + mb->mb_size + sizeof(block_t) == (char *)next) {
    TAILQ_REMOVE(ma_freeblks, next, mb_list);
    mb->mb_size = mb->mb_size + next->mb_size + sizeof(block_t);
  }
}

static void add_free_memory_block(arena_t *ma, block_t *mb, size_t total_size) {
  /* Unpoison and setup the header */
  kasan_mark_valid(mb, sizeof(block_t));
  mb->mb_magic = MB_MAGIC;
  mb->mb_size = total_size - sizeof(block_t);
  /* Poison the data */
  kasan_mark_invalid(mb->mb_data, mb->mb_size, KASAN_CODE_KMALLOC_FREED);

  /* If it's the first block, we simply add it. */
  if (TAILQ_EMPTY(&ma->ma_freeblks)) {
    TAILQ_INSERT_HEAD(&ma->ma_freeblks, mb, mb_list);
    return;
  }

  /* It's not the first block, so we insert it in a sorted fashion. */
  block_t *current = NULL;
  block_t *best_so_far = NULL; /* mb can be inserted after this entry. */

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
  if (size < sizeof(arena_t))
    return;

  memset((void *)start, 0, sizeof(arena_t));
  arena_t *ma = (void *)start;

  TAILQ_INSERT_HEAD(&arena_list, ma, ma_list);
  ma->ma_size = size - sizeof(arena_t);
  ma->ma_magic = MB_MAGIC;

  TAILQ_INIT(&ma->ma_freeblks);

  /* Adding the first free block that covers all the remaining arena_size. */
  block_t *mb = (block_t *)((char *)ma + sizeof(arena_t));
  size_t block_size = size - sizeof(arena_t);
  add_free_memory_block(ma, mb, block_size);
}

static int kmalloc_add_pages(kmalloc_pool_t *mp, size_t size) {
  assert(mtx_owned(&arena_lock));

  size = roundup(size, PAGESIZE);

  void *arena = kmem_alloc(size, M_WAITOK);
  if (arena == NULL)
    return EAGAIN;

  kmalloc_add_arena(mp, arena, size);
  klog("add arena %08x - %08x to '%s' kmem pool", arena, arena + size,
       mp->mp_desc);
  return 0;
}

static block_t *find_entry(block_list_t *mb_list, size_t total_size) {
  block_t *current = NULL;
  TAILQ_FOREACH (current, mb_list, mb_list) {
    assert(current->mb_magic == MB_MAGIC);
    if (current->mb_size >= (ssize_t)total_size)
      return current;
  }
  return NULL;
}

static block_t *try_allocating_in_area(arena_t *ma, size_t requested_size) {
  block_t *mb = find_entry(&ma->ma_freeblks, requested_size + sizeof(block_t));

  if (!mb) /* No entry has enough space. */
    return NULL;

  TAILQ_REMOVE(&ma->ma_freeblks, mb, mb_list);
  size_t total_size_left = mb->mb_size - requested_size;
  if (total_size_left > sizeof(block_t)) {
    mb->mb_size = -requested_size;
    block_t *new_mb =
      (block_t *)((char *)mb + requested_size + sizeof(block_t));
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

  SCOPED_MTX_LOCK(&arena_lock);

  while (1) {
    /* Search for the first area in the list that has enough space. */
    arena_t *current = NULL;
    size_t requested_size = size;
#if KASAN
    requested_size += redzone_size;
#endif /* !KASAN */
    TAILQ_FOREACH (current, &arena_list, ma_list) {
      assert(current->ma_magic == MB_MAGIC);
      block_t *mb = try_allocating_in_area(current, requested_size);
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

    if (kmalloc_add_pages(mp, size + sizeof(arena_t)))
      panic("memory exhausted in '%s'", mp->mp_desc);
  }
}

static inline block_t *block_of(void *addr) {
  return (block_t *)((char *)addr - sizeof(block_t));
}

static void _kfree(kmalloc_pool_t *mp, void *addr) {
  assert(mtx_owned(&arena_lock));

  block_t *mb = block_of(addr);
  if (mb->mb_magic != MB_MAGIC || mb->mb_size >= 0)
    panic("Memory corruption detected!");

  arena_t *current = NULL;
  TAILQ_FOREACH (current, &arena_list, ma_list) {
    char *start = ((char *)current) + sizeof(arena_t);
    if ((char *)addr >= start && (char *)addr < start + current->ma_size)
      add_free_memory_block(current, mb, abs(mb->mb_size) + sizeof(block_t));
  }
}

void kfree(kmalloc_pool_t *mp, void *addr) {
  if (!addr)
    return;
  SCOPED_MTX_LOCK(&arena_lock);

  kasan_mark_invalid(addr, abs(block_of(addr)->mb_size),
                     KASAN_CODE_KMALLOC_FREED);
  kasan_quar_additem(&mp_quarantine, addr);
#if !KASAN
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

void init_kmalloc(void) {
  TAILQ_INIT(&arena_list);
  mtx_init(&arena_lock, 0);
  kasan_quar_init(&block_quarantine, NULL, (quar_free_t)_kfree);
}

void kmalloc_dump(void) {
  arena_t *arena = NULL;
  TAILQ_FOREACH (arena, &arena_list, ma_list) {
    block_t *block = (void *)arena->ma_data;
    block_t *end = (void *)arena->ma_data + arena->ma_size;

    klog("> malloc_arena %p - %p:", block, end);

    while (block < end) {
      assert(block->mb_magic == MB_MAGIC);
      klog("   %c %p %d", (block->mb_size > 0) ? 'F' : 'U', block,
           (unsigned)abs(block->mb_size));
      block = mb_next(block);
    }
  }
}

KMALLOC_DEFINE(M_TEMP, "temporaries pool");
KMALLOC_DEFINE(M_STR, "strings");
