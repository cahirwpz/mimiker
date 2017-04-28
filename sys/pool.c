#define KL_LOG KL_POOL

#include <stdc.h>
#include <vm.h>
#include <physmem.h>
#include <queue.h>
#include <bitstring.h>
#include <common.h>
#include <klog.h>
#include <mutex.h>
#include <sync.h>
#include <pool.h>

#define PI_MAGIC 0xCAFEBABE
#define PP_FREE 0xDEADBEEF
#define PI_ALIGNMENT sizeof(uint64_t)

#define POOL_DEBUG 0

#if defined(POOL_DEBUG) && POOL_DEBUG > 0
#define debug(...) klog(__VA_ARGS__)
#else
#define debug(...)
#endif

typedef struct pool_slab {
  LIST_ENTRY(pool_slab) ph_slablist; /* pool slab list */
  vm_page_t *ph_page;                 /* page containing this slab */
  uint16_t ph_nused;                  /* # of items in use */
  uint16_t ph_itemsize;               /* size of item, a bit redundant but
                                       * there would be padding anyway and it
                                       * simplifies implementation */
  uint16_t ph_ntotal;                 /* total number of chunks*/
  uint16_t ph_start;                  /* start offset in page */
  bitstr_t ph_bitmap[0];
} pool_slab_t;

typedef struct pool_item {
  uint32_t pi_canary;   /* PI_MAGIC by default, if modified then memory
                         * corruption has occured */
  pool_slab_t *pi_slab; /* pointer to slab containing this item */
  unsigned long pi_data[0] __aligned(PI_ALIGNMENT);
} pool_item_t;

static pool_item_t *pool_item_at(pool_slab_t *slab, unsigned i) {
  return (pool_item_t *)((intptr_t)slab + slab->ph_start +
                         i * (slab->ph_itemsize + sizeof(pool_item_t)));
}

static pool_slab_t *create_slab(pool_t *pool) {
  debug("create_slab: pool = %p, pp_itemsize = %d", pool, pool->pp_itemsize);

  vm_page_t *page = pm_alloc(1);
  pool_slab_t *slab = (pool_slab_t *)page->vaddr;
  slab->ph_page = page;
  slab->ph_nused = 0;
  slab->ph_itemsize = pool->pp_itemsize;

  /*
   * Now we need to calculate maximum possible number of items of given `size`
   * in slab that occupies one page, taking into account space taken by:
   *  - items: ntotal * (sizeof(pool_item_t) + size),
   *  - slab + bitmap: sizeof(pool_slab_t) + bitstr_size(ntotal)
   * With:
   *  - usable = PAGESIZE - sizeof(pool_slab_t)
   *  - itemsize = sizeof(pool_item_t) + size;
   * ... inequation looks as follow:
   * (1) ntotal * itemsize + (ntotal + 7) / 8 <= usable
   * (2) ntotal * 8 * itemsize + ntotal + 7 <= usable * 8
   * (3) ntotal * (8 * itemsize + 1) <= usable * 8 + 7
   * (4) ntotal <= (usable * 8 + 7) / (8 * itemisize + 1)
   */
  unsigned usable = PAGESIZE - sizeof(pool_slab_t);
  unsigned itemsize = sizeof(pool_item_t) + slab->ph_itemsize;

  slab->ph_ntotal = (usable * 8 + 7) / (8 * itemsize + 1);
  slab->ph_nused = 0;
  slab->ph_start =
    align(sizeof(pool_slab_t) + bitstr_size(slab->ph_ntotal), PI_ALIGNMENT);
  memset(slab->ph_bitmap, 0, bitstr_size(slab->ph_ntotal));
  for (int i = 0; i < slab->ph_ntotal; i++) {
    pool_item_t *pi = pool_item_at(slab, i);
    pi->pi_slab = slab;
    pi->pi_canary = PI_MAGIC;
    if (pool->pp_ctor)
      pool->pp_ctor(pi->pi_data, slab->ph_itemsize);
  }
  return slab;
}

static void destroy_slab(pool_t *pool, pool_slab_t *slab) {
  klog("destroy_slab: pool = %p, slab = %p", pool, slab);

  for (int i = 0; i < slab->ph_ntotal; i++) {
    pool_item_t *curr_pi = pool_item_at(slab, i);
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
  pool_item_t *found_pi = pool_item_at(slab, found_idx);

  if (found_pi->pi_canary != PI_MAGIC)
    panic("memory corruption at item %p", found_pi);

  slab->ph_nused++;
  debug("slab_alloc: allocated item %p at slab %p, index %d", found_pi->pi_data,
        found_pi->pi_slab, found_idx);
  return found_pi->pi_data;
}

void pool_init(pool_t *pool, size_t size, pool_ctor_t ctor, pool_dtor_t dtor) {
  pool->pp_itemsize = align(size, PI_ALIGNMENT);
  LIST_INIT(&pool->pp_empty_slabs);
  LIST_INIT(&pool->pp_full_slabs);
  LIST_INIT(&pool->pp_part_slabs);
  pool->pp_ctor = ctor;
  pool->pp_dtor = dtor;
  pool_slab_t *first_slab = create_slab(pool);
  LIST_INSERT_HEAD(&pool->pp_empty_slabs, first_slab, ph_slablist);
  pool->pp_nslabs = 1;
  pool->pp_nitems = first_slab->ph_ntotal;
  pool->pp_align = PI_ALIGNMENT;
  mtx_init(&pool->pp_mtx, MTX_DEF);
  klog("pool_init: initialized new pool at %p (item size = %d)", pool,
       pool->pp_itemsize);
}

static bool is_pool_dead(pool_t *pool) {
  uint32_t *tmp = (uint32_t *)pool;
  for (size_t i = 0; i < sizeof(pool_t) / sizeof(uint32_t); i++)
    if (tmp[i] == PP_FREE)
      return true;
  return false;
}

static void mark_pool_dead(pool_t *pool) {
  uint32_t *tmp = (uint32_t *)pool;
  for (size_t i = 0; i < sizeof(pool_t) / sizeof(uint32_t); i++)
    tmp[i] = PP_FREE;
}

static void destroy_slab_list(pool_t *pool, pool_slab_list_t *slabs) {
  pool_slab_t *it, *next;

  LIST_FOREACH_SAFE(it, slabs, ph_slablist, next) {
    LIST_REMOVE(it, ph_slablist);
    destroy_slab(pool, it);
  }
}

void pool_destroy(pool_t *pool) {
  debug("pool_destroy: pool = %p", pool);

  /* Unfortunately, as for now, there is no way to use mutex here because
   * mark_pool_dead belongs to critical section and at the same time, it erases
   * the mutex, that's why low-level sync functions are used here */

  critical_enter();

  if (is_pool_dead(pool))
    panic("attempt to free dead pool %p", pool);

  destroy_slab_list(pool, &pool->pp_empty_slabs);
  destroy_slab_list(pool, &pool->pp_part_slabs);
  destroy_slab_list(pool, &pool->pp_full_slabs);
  mark_pool_dead(pool);

  critical_leave();

  klog("pool_destroy: destroyed pool at %p", pool);
}

/* TODO: find some use for flags */
void *pool_alloc(pool_t *pool, __unused unsigned flags) {
  debug("pool_alloc: pool=%p", pool);

  mtx_scoped_lock(&pool->pp_mtx);

  if (is_pool_dead(pool))
    panic("operation on dead pool %p", pool);

  pool_slab_t *slab_to_use;
  if (pool->pp_nitems) {
    pool_slab_list_t *pool_list = LIST_EMPTY(&pool->pp_part_slabs)
                                    ? &pool->pp_empty_slabs
                                    : &pool->pp_part_slabs;
    slab_to_use = LIST_FIRST(pool_list);
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
    slab_to_use->ph_nused < slab_to_use->ph_ntotal ? &pool->pp_part_slabs
                                                   : &pool->pp_full_slabs;
  LIST_INSERT_HEAD(slab_list_to_insert, slab_to_use, ph_slablist);
  return p;
}

/* TODO: destroy empty slabs when their number reaches a certain threshold
 * (maybe leave one) */
void pool_free(pool_t *pool, void *ptr) {
  debug("pool_free: pool = %p, ptr = %p", pool, ptr);

  mtx_scoped_lock(&pool->pp_mtx);

  if (is_pool_dead(pool))
    panic("operation on dead pool %p", pool);

  pool_item_t *curr_pi = ptr - sizeof(pool_item_t);
  if (curr_pi->pi_canary != PI_MAGIC)
    panic("memory corruption at item %p", curr_pi);

  pool_slab_t *curr_slab = curr_pi->pi_slab;

  /* We need to find index of curr_pi in the bitmap, it can be easily derived
   * from obvious equation curr_pi = curr_slab + curr_slab->ph_start +
   * index * (curr_slab->ph_itemsize + sizeof(pool_item_t)) */
  intptr_t index = ((intptr_t)curr_pi - (intptr_t)curr_slab -
                    (intptr_t)(curr_slab->ph_start)) /
                   ((curr_slab->ph_itemsize) + sizeof(pool_item_t));
  bitstr_t *bitmap = curr_slab->ph_bitmap;

  if (!bit_test(bitmap, index))
    panic("double free at item %p", ptr);

  bit_clear(bitmap, index);
  LIST_REMOVE(curr_slab, ph_slablist);
  curr_slab->ph_nused--;
  pool->pp_nitems++;
  pool_slab_list_t *slab_list_to_insert =
    curr_slab->ph_nused ? &pool->pp_part_slabs : &pool->pp_empty_slabs;
  LIST_INSERT_HEAD(slab_list_to_insert, curr_slab, ph_slablist);

  debug("pool_free: freed item %p at slab %p, index %d", ptr, curr_slab, index);
}
