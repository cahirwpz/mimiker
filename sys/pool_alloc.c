#include <stdc.h>
#include <vm.h>
#include <physmem.h>
#include <queue.h>
#include <bitstring.h>
#include <common.h>
#include <pool_alloc.h>

#define PI_MAGIC_WORD 0xcafebabe

#define PP_FREED_WORD 0xdeadbeef

#define GET_PI_AT_IDX(slab, i, size)                                           \
  (pool_item_t *)((unsigned long)slab + (slab->ph_start) +                     \
                  (i) * (size + sizeof(pool_item_t)))

#define PALLOC_DEBUG 0

#if defined(PALLOC_DEBUG) && PALLOC_DEBUG > 0
#define PALLOC_DEBUG_PRINT(text, args...)                                      \
  kprintf("PALLOC_DEBUG: %s:%d:%s(): " text, __FILE__, __LINE__, __func__,     \
          ##args)
#else
#define PALLOC_DEBUG_PRINT(text, args...)
#endif

typedef struct pool_slab {
  LIST_ENTRY(pool_slab) ph_slablist; /* pool slab list */
  vm_page_t *ph_page;                /* page containing this slab*/
  uint16_t ph_nused;                 /* # of items in use */
  uint16_t ph_nfree;  /* # of free (and available) items, a bit redundant but
                         there would be padding abyway*/
  uint16_t ph_ntotal; /* total number of chunks*/
  uint16_t ph_start;  /* start offset in page */
  bitstr_t ph_bitmap[0];
} pool_slab_t;

typedef struct pool_item {
  unsigned long pi_guard_number; /* PI_MAGIC_WORD by default, normally isn't */
  /* changed, so if it has another value, memory */
  /* corruption has taken place */
  pool_slab_t *pi_slab; /* pointer to slab containing this item */
  unsigned long pi_data[0];
} pool_item_t;

void pool_default_destructor(__unused void *buf, __unused size_t size){};

static pool_slab_t *create_slab(pool_t *pool) {
  PALLOC_DEBUG_PRINT("Entering create_slab, size=%d\n", pool->pp_itemsize);
  vm_page_t *page_for_slab = pm_alloc(1);
  pool_slab_t *slab = (pool_slab_t *)page_for_slab->vaddr;
  slab->ph_page = page_for_slab;
  slab->ph_nused = 0;
  slab->ph_ntotal = (PAGESIZE - sizeof(pool_slab_t) - 3) /
                    (sizeof(pool_item_t) + pool->pp_itemsize +
                     1); /* sizeof(pool_slab_t)+n*(sizeof(pool_item_t)+size+1)+3
                   <= PAGESIZE, 3 is maximum number of padding bytes
                   for a bitmap */

  slab->ph_nfree = slab->ph_ntotal;
  slab->ph_start = sizeof(pool_slab_t) + align(slab->ph_ntotal, 4);
  memset(slab->ph_bitmap, 0, bitstr_size(slab->ph_ntotal));
  for (int i = 0; i < slab->ph_ntotal; i++) {
    pool_item_t *curr_pi = GET_PI_AT_IDX(slab, i, pool->pp_itemsize);
    curr_pi->pi_slab = slab;
    curr_pi->pi_guard_number = PI_MAGIC_WORD;
    (pool->pp_constructor)(curr_pi->pi_data, pool->pp_itemsize);
  }
  return slab;
}

static void destroy_slab(pool_slab_t *slab, pool_t *pool) {
  PALLOC_DEBUG_PRINT("Entering destroy_slab\n");
  for (int i = 0; i < slab->ph_ntotal; i++) {
    pool_item_t *curr_pi = GET_PI_AT_IDX(slab, i, pool->pp_itemsize);
    (pool->pp_destructor)(curr_pi->pi_data, pool->pp_itemsize);
  }
  pm_free(slab->ph_page);
}

static void *slab_alloc(pool_slab_t *slab, size_t size) {
  bitstr_t *bitmap = slab->ph_bitmap;
  int i = 0;
  bit_ffc(bitmap, slab->ph_ntotal, &i);
  pool_item_t *found_pi = GET_PI_AT_IDX(slab, i, size);
  if (found_pi->pi_guard_number != PI_MAGIC_WORD) {
    panic("memory corruption at item 0x%lx\n", (unsigned long)found_pi);
  }
  slab->ph_nused++;
  slab->ph_nfree--;
  bit_set(bitmap, i);
  PALLOC_DEBUG_PRINT("Allocated an item (0x%lx) at slab 0x%lx, index %d\n",
                     (unsigned long)found_pi->pi_data,
                     (unsigned long)found_pi->pi_slab, i);
  return found_pi->pi_data;
}

void pool_init(pool_t *pool, size_t size, void (*constructor)(void *, size_t),
               void (*destructor)(void *, size_t)) {
  PALLOC_DEBUG_PRINT("Entering pool_init\n");
  size = align(size, 4); /* Align to 32-bit word */
  pool->pp_itemsize = size;
  LIST_INIT(&pool->pp_empty_slabs);
  LIST_INIT(&pool->pp_full_slabs);
  LIST_INIT(&pool->pp_part_slabs);
  pool->pp_constructor = constructor;
  pool->pp_destructor = destructor;
  pool_slab_t *first_slab = create_slab(pool);
  LIST_INSERT_HEAD(&pool->pp_empty_slabs, first_slab, ph_slablist);
  pool->pp_nslabs = 1;
  pool->pp_nitems = first_slab->ph_ntotal;
  PALLOC_DEBUG_PRINT("Initialized new pool at 0x%lx\n", (unsigned long)pool);
}

void pool_destroy(pool_t *pool) {

  PALLOC_DEBUG_PRINT("Entering pool_destroy\n");

  unsigned long *tmp = (unsigned long *)pool;

  for (size_t i = 0; i < sizeof(pool_t) / sizeof(unsigned long); i++) {
    if (tmp[i] == PP_FREED_WORD) {
      panic("double free at pool 0x%lx\n", (unsigned long)pool);
    }
  }

  PALLOC_DEBUG_PRINT("Destroying empty slabs\n");
  pool_slab_t *it = LIST_FIRST(&pool->pp_empty_slabs);
  while (it) {
    pool_slab_t *next = LIST_NEXT(it, ph_slablist);
    LIST_REMOVE(it, ph_slablist);
    destroy_slab(it, pool);
    it = next;
  }

  PALLOC_DEBUG_PRINT("Destroying partially filled slabs\n");
  it = LIST_FIRST(&pool->pp_part_slabs);
  while (it) {
    pool_slab_t *next = LIST_NEXT(it, ph_slablist);
    LIST_REMOVE(it, ph_slablist);
    destroy_slab(it, pool);
    it = next;
  }

  PALLOC_DEBUG_PRINT("Destroying full slabs\n");
  it = LIST_FIRST(&pool->pp_full_slabs);
  while (it) {
    pool_slab_t *next = LIST_NEXT(it, ph_slablist);
    LIST_REMOVE(it, ph_slablist);
    destroy_slab(it, pool);
    it = next;
  }

  for (size_t i = 0; i < sizeof(pool_t) / sizeof(unsigned long); i++) {
    tmp[i] = PP_FREED_WORD;
  }
  PALLOC_DEBUG_PRINT("Destroyed pool at 0x%lx\n", (unsigned long)pool);
}

void *
pool_alloc(pool_t *pool,
           __unused uint32_t
             flags) { /* TODO: implement different flags (no idea which, tbh) */
  PALLOC_DEBUG_PRINT("Entering pool_alloc\n");

  unsigned long *tmp = (unsigned long *)pool;
  for (size_t i = 0; i < sizeof(pool_t) / sizeof(unsigned long); i++) {
    if (tmp[i] == PP_FREED_WORD) {
      panic("operation on a free pool 0x%lx\n", (unsigned long)pool);
    }
  }

  pool_slab_t *slab_to_use;
  if (pool->pp_nitems) {
    slab_to_use = LIST_EMPTY(&pool->pp_part_slabs)
                    ? LIST_FIRST(&pool->pp_empty_slabs)
                    : LIST_FIRST(&pool->pp_part_slabs);
    LIST_REMOVE(slab_to_use, ph_slablist);
  } else {
    slab_to_use = create_slab(pool);
    pool->pp_nitems += slab_to_use->ph_ntotal;
    pool->pp_nslabs++;
    PALLOC_DEBUG_PRINT("Growing pool 0x%lx\n", (unsigned long)pool);
  }
  void *p = slab_alloc(slab_to_use, pool->pp_itemsize);
  pool->pp_nitems--;
  pool_slab_list_t *slab_list_to_insert =
    slab_to_use->ph_nfree ? &pool->pp_part_slabs : &pool->pp_full_slabs;
  LIST_INSERT_HEAD(slab_list_to_insert, slab_to_use, ph_slablist);

  return p;
}
void pool_free(pool_t *pool,
               void *ptr) { /* TODO: destroy empty slabs when their number
                               reaches a certain threshold (maybe leave one) */
  PALLOC_DEBUG_PRINT("Entering pool_free\n");

  unsigned long *tmp = (unsigned long *)pool;
  for (size_t i = 0; i < sizeof(pool_t) / sizeof(unsigned long); i++) {
    if (tmp[i] == PP_FREED_WORD) {
      panic("operation on a free pool 0x%lx\n", (unsigned long)pool);
    }
  }

  pool_item_t *curr_pi = ptr - sizeof(pool_item_t);
  if (curr_pi->pi_guard_number != PI_MAGIC_WORD) {
    panic("memory corruption at item 0x%lx\n", (unsigned long)curr_pi);
  }
  pool_slab_t *curr_slab = curr_pi->pi_slab;
  unsigned long index = ((unsigned long)curr_pi - (unsigned long)curr_slab -
                         (unsigned long)(curr_slab->ph_start)) /
                        ((pool->pp_itemsize) + sizeof(pool_item_t));
  bitstr_t *bitmap = curr_slab->ph_bitmap;
  if (!bit_test(bitmap, index)) {
    panic("double free at item 0x%lx\n", (unsigned long)ptr);
  }

  bit_clear(bitmap, index);
  LIST_REMOVE(curr_slab, ph_slablist);
  curr_slab->ph_nused--;
  curr_slab->ph_nfree++;
  if (!(curr_slab->ph_nused)) {
    LIST_INSERT_HEAD(&pool->pp_empty_slabs, curr_slab, ph_slablist);
  } else {
    LIST_INSERT_HEAD(&pool->pp_part_slabs, curr_slab, ph_slablist);
  }
  PALLOC_DEBUG_PRINT("Freed an item (0x%lx) at slab 0x%lx, index %ld\n",
                     (unsigned long)ptr, (unsigned long)curr_slab, index);
}
