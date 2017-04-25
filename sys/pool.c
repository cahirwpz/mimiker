#include <stdc.h>
#include <vm.h>
#include <physmem.h>
#include <queue.h>
#include <bitstring.h>
#include <common.h>
#include <klog.h>
#include <pool.h>

#define PI_MAGIC_WORD 0xcafebabe
#define PP_FREED_WORD 0xdeadbeef

#define PALLOC_DEBUG 0

#if defined(PALLOC_DEBUG) && PALLOC_DEBUG > 0
#define debug_log(text, args...)                                               \
  klog("DEBUG: %s:%d:%s(): " text, __FILE__, __LINE__, __func__, ##args)
#else
#define debug_log(text, args...)
#endif

typedef struct pool_slab {
  LIST_ENTRY(pool_slab) ph_slablist; /* pool slab list */
  vm_page_t *ph_page;                /* page containing this slab */
  uint16_t ph_nused;                 /* # of items in use */
  uint16_t ph_itemsize;  /* size of item, a bit redundant but
                         there would be padding anyway and it simplifies implementation */
  uint16_t ph_ntotal; /* total number of chunks*/
  uint16_t ph_start;  /* start offset in page */
  bitstr_t ph_bitmap[0];
} pool_slab_t;

typedef struct pool_item {
  uint32_t pi_canary;   /* PI_MAGIC_WORD by default, if modified then memory
                           corruption has occured */
  pool_slab_t *pi_slab; /* pointer to slab containing this item */
  unsigned long pi_data[0];
} pool_item_t;

static pool_item_t *get_pi_at_idx(pool_slab_t *slab, unsigned i, size_t size) {
  return (pool_item_t *)((intptr_t)slab + slab->ph_start +
                         i * (size + sizeof(pool_item_t)));
}

static pool_slab_t *create_slab(pool_t *pool) {
  debug_log("create_slab: pool = %p, pp_itemsize = %d", pool,
            pool->pp_itemsize);
  vm_page_t *page_for_slab = pm_alloc(1);
  pool_slab_t *slab = (pool_slab_t *)page_for_slab->vaddr;
  slab->ph_page = page_for_slab;
  slab->ph_nused = 0;
  slab->ph_itemsize = pool->pp_itemsize;

  /* Now we need to calculate maximum number of items (slab->ph_ntotal as n) in
   * slab, taking into account space occupied by items (which is
   * n*(sizeof(pool_item_t)+size)), size of bitmap (which is ((n+31)/32)*4 ==
   * howmany(n, 32)*4) and size of slab structure (which is
   * sizeof(pool_slab_t)). Obviously, all this should fit into page, resulting
   * in inequation
   * sizeof(pool_slab_t) + n * (sizeof(pool_item_t) + size)
   * + ((n + 31) / 32) * 4 <= PAGESIZE, from which n can be derived pretty
   * easily*/
  slab->ph_ntotal = (8 * (PAGESIZE - sizeof(pool_slab_t)) - 31) /
                    (8 * (sizeof(pool_item_t) + slab->ph_itemsize) + 1);

  slab->ph_nused = 0;
  slab->ph_start = sizeof(pool_slab_t) + howmany(slab->ph_ntotal, 32) * 4;
  memset(slab->ph_bitmap, 0, bitstr_size(slab->ph_ntotal));
  for (int i = 0; i < slab->ph_ntotal; i++) {
    pool_item_t *curr_pi = get_pi_at_idx(slab, i, slab->ph_itemsize);
    debug_log("Still alive, are we? %d", i);
    curr_pi->pi_slab = slab;
    curr_pi->pi_canary = PI_MAGIC_WORD;
    if (pool->pp_ctor)
      pool->pp_ctor(curr_pi->pi_data, slab->ph_itemsize);
  }
  return slab;
}

static void destroy_slab(pool_t *pool, pool_slab_t *slab) {
  klog("destroy_slab: pool = %p, slab = %p", pool, slab);
  for (int i = 0; i < slab->ph_ntotal; i++) {
    pool_item_t *curr_pi = get_pi_at_idx(slab, i, slab->ph_itemsize);
    if (pool->pp_dtor)
      pool->pp_dtor(curr_pi->pi_data, slab->ph_itemsize);
  }
  pm_free(slab->ph_page);
}

static void *slab_alloc(pool_slab_t *slab) {
  assert(slab->ph_nused < slab->ph_ntotal);
  int found_idx = 0;
  bit_ffc(slab->ph_bitmap, slab->ph_ntotal, &found_idx);
  bit_set(slab->ph_bitmap, found_idx);
  pool_item_t *found_pi = get_pi_at_idx(slab, found_idx, slab->ph_itemsize);
  if (found_pi->pi_canary != PI_MAGIC_WORD)
    panic("memory corruption at item %p", found_pi);
  slab->ph_nused++;
  debug_log("slab_alloc: allocated item %p at slab %p, index %d",
            found_pi->pi_data, found_pi->pi_slab, found_idx);
  return found_pi->pi_data;
}

void pool_init(pool_t *pool, size_t size, pool_ctor_t ctor, pool_dtor_t dtor) {
  size = align(size, 4); /* Align to 32-bit word */
  pool->pp_itemsize = size;
  LIST_INIT(&pool->pp_empty_slabs);
  LIST_INIT(&pool->pp_full_slabs);
  LIST_INIT(&pool->pp_part_slabs);
  pool->pp_ctor = ctor;
  pool->pp_dtor = dtor;
  pool_slab_t *first_slab = create_slab(pool);
  LIST_INSERT_HEAD(&pool->pp_empty_slabs, first_slab, ph_slablist);
  pool->pp_nslabs = 1;
  pool->pp_nitems = first_slab->ph_ntotal;
  pool->pp_align = 4;
  klog("pool_init: initialized new pool at %p (item size = %d)", pool, size);
}

static bool is_pool_dead(pool_t *pool) {
  unsigned long *tmp = (unsigned long *)pool;
  for (size_t i = 0; i < sizeof(pool_t) / sizeof(unsigned long); i++) {
    if (tmp[i] == PP_FREED_WORD)
      return true;
  }
  return false;
}

static void mark_pool_dead(pool_t *pool) {
  unsigned long *tmp = (unsigned long *)pool;
  for (size_t i = 0; i < sizeof(pool_t) / sizeof(unsigned long); i++) {
    tmp[i] = PP_FREED_WORD;
  }
}

void pool_destroy(pool_t *pool) {
  debug_log("pool_destroy: pool = %p", pool);

  if (is_pool_dead(pool))
    panic("attempt to free dead pool %p", pool);

  pool_slab_t *it, *next;

  LIST_FOREACH_SAFE(it, &pool->pp_empty_slabs, ph_slablist, next) {
    LIST_REMOVE(it, ph_slablist);
    destroy_slab(pool, it);
  }

  LIST_FOREACH_SAFE(it, &pool->pp_part_slabs, ph_slablist, next) {
    LIST_REMOVE(it, ph_slablist);
    destroy_slab(pool, it);
  }

  LIST_FOREACH_SAFE(it, &pool->pp_full_slabs, ph_slablist, next) {
    LIST_REMOVE(it, ph_slablist);
    destroy_slab(pool, it);
  }

  mark_pool_dead(pool);

  klog("pool_destroy: destroyed pool at %p", pool);
}

/* TODO: find some use for flags */
void *pool_alloc(pool_t *pool, __unused unsigned flags) {
  debug_log("pool_alloc: pool=%p", pool);

  if (is_pool_dead(pool))
    panic("operation on dead pool %p", pool);

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
    klog("pool_alloc: growing pool at %p", pool);
  }
  void *p = slab_alloc(slab_to_use);
  pool->pp_nitems--;
  pool_slab_list_t *slab_list_to_insert =
    slab_to_use->ph_nused < slab_to_use->ph_ntotal ? &pool->pp_part_slabs : &pool->pp_full_slabs;
  LIST_INSERT_HEAD(slab_list_to_insert, slab_to_use, ph_slablist);
  return p;
}

/* TODO: destroy empty slabs when their number reaches a certain threshold
 * (maybe leave one) */
void pool_free(pool_t *pool, void *ptr) {
  debug_log("pool_free: pool = %p, ptr = %p", pool, ptr);

  if (is_pool_dead(pool))
    panic("operation on dead pool %p", pool);

  pool_item_t *curr_pi = ptr - sizeof(pool_item_t);
  if (curr_pi->pi_canary != PI_MAGIC_WORD) {
    panic("memory corruption at item %p", curr_pi);
  }
  pool_slab_t *curr_slab = curr_pi->pi_slab;
  intptr_t index = ((intptr_t)curr_pi - (intptr_t)curr_slab -
                    (intptr_t)(curr_slab->ph_start)) /
                   ((curr_slab->ph_itemsize) + sizeof(pool_item_t));
  bitstr_t *bitmap = curr_slab->ph_bitmap;
  if (!bit_test(bitmap, index))
    panic("double free at item %p", ptr);
  bit_clear(bitmap, index);
  LIST_REMOVE(curr_slab, ph_slablist);
  curr_slab->ph_nused--;
  pool_slab_list_t *slab_list_to_insert =
    curr_slab->ph_nused ? &pool->pp_part_slabs : &pool->pp_empty_slabs;
  LIST_INSERT_HEAD(slab_list_to_insert, curr_slab, ph_slablist);
  debug_log("pool_free: freed item %p at slab %p, index %d", ptr, curr_slab,
            index);
}
