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

#define ALIVE 0xFACEFEED
#define DEAD 0xDEADC0DE

#define PI_MAGIC 0xCAFEBABE
#define PI_ALIGNMENT sizeof(uint64_t)

#define POOL_DEBUG 0

#if defined(POOL_DEBUG) && POOL_DEBUG > 0
#define debug(...) klog(__VA_ARGS__)
#else
#define debug(...)
#endif

typedef struct pool_slab {
  uint32_t ph_state;                 /* set to ALIVE or DEAD */
  LIST_ENTRY(pool_slab) ph_slablist; /* pool slab list */
  vm_page_t *ph_page;                /* page containing this slab */
  uint16_t ph_nused;                 /* # of items in use */
  uint16_t ph_ntotal;                /* total number of chunks */
  size_t ph_itemsize;                /* total size of item (with header) */
  void *ph_items;                    /* ptr to array of items after bitmap */
  bitstr_t ph_bitmap[0];
} pool_slab_t;

typedef struct pool_item {
  uint32_t pi_canary;   /* PI_MAGIC by default, if modified then memory
                         * corruption has occured */
  pool_slab_t *pi_slab; /* pointer to slab containing this item */
  unsigned long pi_data[0] __aligned(PI_ALIGNMENT);
} pool_item_t;

static pool_item_t *slab_item_at(pool_slab_t *slab, unsigned i) {
  return (pool_item_t *)(slab->ph_items + i * slab->ph_itemsize);
}

static unsigned slab_index_of(pool_slab_t *slab, pool_item_t *item) {
  return ((intptr_t)item - (intptr_t)slab->ph_items) / slab->ph_itemsize;
}

static pool_slab_t *create_slab(pool_t *pool) {
  debug("create_slab: pool = %p, pp_itemsize = %d", pool, pool->pp_itemsize);

  vm_page_t *page = pm_alloc(1);
  pool_slab_t *slab = (pool_slab_t *)page->vaddr;
  slab->ph_state = ALIVE;
  slab->ph_page = page;
  slab->ph_nused = 0;
  slab->ph_itemsize = pool->pp_itemsize + sizeof(pool_item_t);

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
  slab->ph_ntotal = (usable * 8 + 7) / (8 * slab->ph_itemsize + 1);
  slab->ph_nused = 0;

  unsigned header = sizeof(pool_slab_t) + bitstr_size(slab->ph_ntotal);
  slab->ph_items = (void *)slab + align(header, PI_ALIGNMENT);
  memset(slab->ph_bitmap, 0, bitstr_size(slab->ph_ntotal));

  for (int i = 0; i < slab->ph_ntotal; i++) {
    pool_item_t *pi = slab_item_at(slab, i);
    pi->pi_slab = slab;
    pi->pi_canary = PI_MAGIC;
    if (pool->pp_ctor)
      pool->pp_ctor(pi->pi_data);
  }

  return slab;
}

static void destroy_slab(pool_t *pool, pool_slab_t *slab) {
  klog("destroy_slab: pool = %p, slab = %p", pool, slab);

  for (int i = 0; i < slab->ph_ntotal; i++) {
    pool_item_t *curr_pi = slab_item_at(slab, i);
    if (pool->pp_dtor)
      pool->pp_dtor(curr_pi->pi_data);
  }

  slab->ph_state = DEAD;
  pm_free(slab->ph_page);
}

static void *slab_alloc(pool_slab_t *slab) {
  assert(slab->ph_state == ALIVE);
  assert(slab->ph_nused < slab->ph_ntotal);

  int i = 0;
  bit_ffc(slab->ph_bitmap, slab->ph_ntotal, &i);
  bit_set(slab->ph_bitmap, i);
  pool_item_t *pi = slab_item_at(slab, i);
  assert(pi->pi_canary == PI_MAGIC);
  slab->ph_nused++;
  debug("slab_alloc: allocated item %p at slab %p, index %d", pi->pi_data,
        pi->pi_slab, i);

  return pi->pi_data;
}

void pool_init(pool_t *pool, size_t size, pool_ctor_t ctor, pool_dtor_t dtor) {
  pool->pp_itemsize = align(size, PI_ALIGNMENT);
  LIST_INIT(&pool->pp_empty_slabs);
  LIST_INIT(&pool->pp_full_slabs);
  LIST_INIT(&pool->pp_part_slabs);
  pool->pp_ctor = ctor;
  pool->pp_dtor = dtor;
  pool_slab_t *slab = create_slab(pool);
  LIST_INSERT_HEAD(&pool->pp_empty_slabs, slab, ph_slablist);
  pool->pp_nslabs = 1;
  pool->pp_nitems = slab->ph_ntotal;
  pool->pp_align = PI_ALIGNMENT;
  mtx_init(&pool->pp_mtx, MTX_DEF);
  pool->pp_state = ALIVE;
  klog("pool_init: initialized new pool at %p (item size = %d)", pool,
       pool->pp_itemsize);
}

static void destroy_slab_list(pool_t *pool, pool_slabs_t *slabs) {
  pool_slab_t *it, *next;

  LIST_FOREACH_SAFE(it, slabs, ph_slablist, next) {
    LIST_REMOVE(it, ph_slablist);
    destroy_slab(pool, it);
  }
}

void pool_destroy(pool_t *pool) {
  debug("pool_destroy: pool = %p", pool);

  /* TODO: there is no way to use pool's mutex here because it could already got
   * deallocated and we have no method of marking dead mutexes, that's why
   * low-level sync functions are used here. */
  IN_CRITICAL_SECTION () {
    assert(pool->pp_state == ALIVE);
    pool->pp_state = DEAD;
  }

  destroy_slab_list(pool, &pool->pp_empty_slabs);
  destroy_slab_list(pool, &pool->pp_part_slabs);
  destroy_slab_list(pool, &pool->pp_full_slabs);

  klog("pool_destroy: destroyed pool at %p", pool);
}

/* TODO: find some use for flags */
void *pool_alloc(pool_t *pool, __unused unsigned flags) {
  debug("pool_alloc: pool=%p", pool);

  assert(pool->pp_state == ALIVE);

  void *p = NULL;

  WITH_MTX_LOCK (&pool->pp_mtx) {
    pool_slab_t *slab;
    if (pool->pp_nitems) {
      pool_slabs_t *slabs = LIST_EMPTY(&pool->pp_part_slabs)
                              ? &pool->pp_empty_slabs
                              : &pool->pp_part_slabs;
      slab = LIST_FIRST(slabs);
      LIST_REMOVE(slab, ph_slablist);
    } else {
      slab = create_slab(pool);
      pool->pp_nitems += slab->ph_ntotal;
      pool->pp_nslabs++;
      klog("pool_alloc: growing pool at %p", pool);
    }

    p = slab_alloc(slab);
    pool->pp_nitems--;
    pool_slabs_t *slabs = (slab->ph_nused < slab->ph_ntotal)
                            ? &pool->pp_part_slabs
                            : &pool->pp_full_slabs;
    LIST_INSERT_HEAD(slabs, slab, ph_slablist);
  }

  return p;
}

/* TODO: destroy empty slabs when their number reaches a certain threshold
 * (maybe leave one) */
void pool_free(pool_t *pool, void *ptr) {
  debug("pool_free: pool = %p, ptr = %p", pool, ptr);

  assert(pool->pp_state == ALIVE);

  WITH_MTX_LOCK (&pool->pp_mtx) {
    pool_item_t *pi = ptr - sizeof(pool_item_t);
    assert(pi->pi_canary == PI_MAGIC);
    pool_slab_t *slab = pi->pi_slab;
    assert(slab->ph_state == ALIVE);

    unsigned index = slab_index_of(slab, pi);
    bitstr_t *bitmap = slab->ph_bitmap;

    assert(bit_test(bitmap, index));

    bit_clear(bitmap, index);
    LIST_REMOVE(slab, ph_slablist);
    slab->ph_nused--;
    pool->pp_nitems++;
    pool_slabs_t *slabs =
      slab->ph_nused ? &pool->pp_part_slabs : &pool->pp_empty_slabs;
    LIST_INSERT_HEAD(slabs, slab, ph_slablist);
  }

  debug("pool_free: freed item %p at slab %p, index %d", ptr, slab, index);
}
