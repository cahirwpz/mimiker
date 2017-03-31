#include <stdc.h>
#include <malloc.h>
#include <physmem.h>
#include <queue.h>

/* Leave synchronization markers in case we need it. */
#define cs_enter()
#define cs_leave()

/* The end of the kernel's .bss section. Provided by the linker. */
extern uint8_t __ebss[];
/* Limit for the end of kernel's bss. Provided by the linker. */
extern uint8_t __kernel_end[];

static struct {
  void *ptr;
  void *end;
} sbrk = {__ebss, __kernel_end};

/*
void kernel_brk(void *addr) {
  cs_enter();
  void *ptr = sbrk.ptr;
  addr = (void *)((intptr_t)addr & -sizeof(uint64_t));
  assert((intptr_t)__ebss <= (intptr_t)addr);
  assert((intptr_t)addr <= (intptr_t)sbrk.end);
  sbrk.ptr = addr;
  cs_leave();
  if (addr > ptr)
    bzero(ptr, (intptr_t)addr - (intptr_t)ptr);
}
*/

void *kernel_sbrk(size_t size) {
  cs_enter();
  void *ptr = sbrk.ptr;
  size = roundup(size, sizeof(uint64_t));
  assert(ptr + size <= sbrk.end);
  sbrk.ptr += size;
  cs_leave();
  bzero(ptr, size);
  return ptr;
}

/*
  TODO:
  - use the mp_next field of malloc_pool
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

static void kmalloc_add_arena(malloc_pool_t *mp, vm_addr_t start,
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

static void kmalloc_add_pages(malloc_pool_t *mp, unsigned pages) {
  vm_page_t *pg = pm_alloc(pages);
  kmalloc_add_arena(mp, PG_VADDR_START(pg), PG_SIZE(pg));
}

static mem_block_t *find_entry(struct mb_list *mb_list, size_t total_size) {
  mem_block_t *current = NULL;
  TAILQ_FOREACH (current, mb_list, mb_list) {
    assert(current->mb_magic == MB_MAGIC);
    if (current->mb_size >= total_size)
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

void kmalloc_init(malloc_pool_t *mp, uint32_t pages, uint32_t pages_limit) {
  TAILQ_INIT(&mp->mp_arena);
  mtx_init(&mp->mutex, MTX_RECURSE);
  kmalloc_add_pages(mp, pages);
  mp->pages_used = pages;
  mp->pages_limit = pages_limit;
}

void *kmalloc(malloc_pool_t *mp, size_t size, uint16_t flags) {
  size_t size_aligned = align(size, MB_ALIGNMENT);

  /* Search for the first area in the list that has enough space. */
  mem_arena_t *current = NULL;
  mtx_lock(&mp->mutex);
  TAILQ_FOREACH (current, &mp->mp_arena, ma_list) {
    assert(current->ma_magic == MB_MAGIC);

    mem_block_t *mb = try_allocating_in_area(current, size_aligned);

    if (mb) {
      if (flags == M_ZERO)
        memset(mb->mb_data, 0, size);
      mtx_unlock(&mp->mutex);
      return mb->mb_data;
    }
  }
  /* Couldn't find any continuous memory with the requested size. */
  if (mp->pages_used < mp->pages_limit) {
    kmalloc_add_pages(mp, 1);
    mp->pages_used++;
    void *ret = kmalloc(mp, size, flags);
    mtx_unlock(&mp->mutex);
    return ret;
  }

  mtx_unlock(&mp->mutex);
  if (flags & M_NOWAIT)
    return NULL;

  panic("memory exhausted in '%s'", mp->mp_desc);
}

void kfree(malloc_pool_t *mp, void *addr) {
  mem_block_t *mb = (mem_block_t *)(((char *)addr) - sizeof(mem_block_t));

  if (mb->mb_magic != MB_MAGIC || mp->mp_magic != MB_MAGIC || mb->mb_size >= 0)
    panic("Memory corruption detected!");

  mem_arena_t *current = NULL;
  mtx_lock(&mp->mutex);
  TAILQ_FOREACH (current, &mp->mp_arena, ma_list) {
    char *start = ((char *)current) + sizeof(mem_arena_t);
    if ((char *)addr >= start && (char *)addr < start + current->ma_size)
      add_free_memory_block(current, mb,
                            abs(mb->mb_size) + sizeof(mem_block_t));
  }
  mtx_unlock(&mp->mutex);
}

char *kstrndup(malloc_pool_t *mp, const char *s, size_t maxlen) {
  size_t n = strnlen(s, maxlen) + 1;
  char *copy = kmalloc(mp, n, M_ZERO);
  memcpy(copy, s, n);
  return copy;
}

void kmalloc_dump(malloc_pool_t *mp) {
  mem_arena_t *arena = NULL;
  kprintf("[kmalloc] malloc_pool at %p:\n", mp);
  mtx_lock(&mp->mutex);
  TAILQ_FOREACH (arena, &mp->mp_arena, ma_list) {
    mem_block_t *block = (void *)arena->ma_data;
    mem_block_t *end = (void *)arena->ma_data + arena->ma_size;

    kprintf("[kmalloc]  malloc_arena %p - %p:\n", block, end);

    while (block < end) {
      assert(block->mb_magic == MB_MAGIC);
      kprintf("[kmalloc]   %c %p %d\n", (block->mb_size > 0) ? 'F' : 'U', block,
              (unsigned)abs(block->mb_size));
      block = mb_next(block);
    }
  }
  mtx_unlock(&mp->mutex);
}
