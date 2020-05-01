#define KL_LOG KL_KMEM
#include <sys/libkern.h>
#include <sys/queue.h>
#include <sys/mimiker.h>
#include <sys/klog.h>
#include <sys/mutex.h>
#include <sys/linker_set.h>
#include <sys/sched.h>
#include <sys/pool.h>
#include <sys/kmem.h>
#include <machine/vm_param.h>
#include <bitstring.h>
#include <sys/kasan.h>

#define INITME 0xC0DECAFE
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

typedef void (*pool_ctor_t)(void *);
typedef void (*pool_dtor_t)(void *);

typedef LIST_HEAD(, pool_slab) pool_slabs_t;

typedef struct pool {
  mtx_t pp_mtx;
  const char *pp_desc;
  unsigned pp_state;
  pool_slabs_t pp_empty_slabs;
  pool_slabs_t pp_full_slabs;
  pool_slabs_t pp_part_slabs; /* partially allocated slabs */
  pool_ctor_t pp_ctor;
  pool_dtor_t pp_dtor;
  size_t pp_itemsize; /* size of item */
  size_t pp_align;    /* (ignored) requested alignment, must be 2^n */
  size_t pp_nslabs;   /* # of slabs allocated */
  size_t pp_nitems;   /* number of available items in pool */
#ifdef KASAN
  size_t pp_redzsize; /* size of redzone after each item */
  quar_t pp_quarantine;
#endif
} pool_t;

typedef struct pool_slab {
  uint32_t ph_state;                 /* set to ALIVE or DEAD */
  LIST_ENTRY(pool_slab) ph_slablist; /* pool slab list */
  uint16_t ph_nused;                 /* # of items in use */
  uint16_t ph_ntotal;                /* total number of chunks */
  size_t ph_size;                    /* size of memory allocated for the slab */
  size_t ph_itemsize; /* total size of item (with header and redzone) */
  void *ph_items;     /* ptr to array of items after bitmap */
  bitstr_t ph_bitmap[0];
} pool_slab_t;

typedef struct pool_item {
  uint32_t pi_canary;   /* PI_MAGIC by default, if modified then memory
                         * corruption has occured */
  pool_slab_t *pi_slab; /* pointer to slab containing this item */
  unsigned long pi_data[0] __aligned(PI_ALIGNMENT);
} pool_item_t;

/* Pool of pool_t objects. */
static pool_t P_POOL[1];

#ifdef KASAN
/* Quarantine increases size of pool structure. More boot pages needed! */
#define POOL_BOOTPAGE_CNT 2
#else
#define POOL_BOOTPAGE_CNT 1
#endif /* !KASAN */
static alignas(PAGESIZE) uint8_t P_POOL_BOOTPAGE[PAGESIZE * POOL_BOOTPAGE_CNT];

static pool_item_t *slab_item_at(pool_slab_t *slab, unsigned i) {
  return (pool_item_t *)(slab->ph_items + i * slab->ph_itemsize);
}

static unsigned slab_index_of(pool_slab_t *slab, pool_item_t *item) {
  return ((intptr_t)item - (intptr_t)slab->ph_items) / slab->ph_itemsize;
}

static size_t align_size(size_t size) {
  return align(size, PI_ALIGNMENT);
}

static void add_slab(pool_t *pool, pool_slab_t *slab, size_t slabsize) {
  assert(mtx_owned(&pool->pp_mtx));

  klog("add slab at %p to '%s' pool", slab, pool->pp_desc);

  slab->ph_state = ALIVE;
  slab->ph_nused = 0;
  slab->ph_size = slabsize;
  slab->ph_itemsize = pool->pp_itemsize + sizeof(pool_item_t);
#ifdef KASAN
  slab->ph_itemsize += pool->pp_redzsize;
#endif /* !KASAN */
  /*
   * Now we need to calculate maximum possible number of items of given `size`
   * in slab that occupies one page, taking into account space taken by:
   *  - items: ntotal * (sizeof(pool_item_t) + size),
   *  - slab + bitmap: sizeof(pool_slab_t) + bitstr_size(ntotal)
   * With:
   *  - usable = slabsize - sizeof(pool_slab_t)
   *  - itemsize = sizeof(pool_item_t) + size;
   * ... inequation looks as follow:
   * (1) ntotal * itemsize + (ntotal + 7) / 8 <= usable
   * (2) ntotal * 8 * itemsize + ntotal + 7 <= usable * 8
   * (3) ntotal * (8 * itemsize + 1) <= usable * 8 + 7
   * (4) ntotal <= (usable * 8 + 7) / (8 * itemisize + 1)
   */
  size_t usable = slabsize - sizeof(pool_slab_t);
  slab->ph_ntotal = (usable * 8 + 7) / (8 * slab->ph_itemsize + 1);
  slab->ph_nused = 0;

  size_t header = sizeof(pool_slab_t) + bitstr_size(slab->ph_ntotal);
  slab->ph_items = (void *)slab + align_size(header);
  memset(slab->ph_bitmap, 0, bitstr_size(slab->ph_ntotal));

  for (int i = 0; i < slab->ph_ntotal; i++) {
    pool_item_t *pi = slab_item_at(slab, i);
    pi->pi_slab = slab;
    pi->pi_canary = PI_MAGIC;
    if (pool->pp_ctor)
      pool->pp_ctor(pi->pi_data);
  }

  LIST_INSERT_HEAD(&pool->pp_empty_slabs, slab, ph_slablist);
  pool->pp_nitems += slab->ph_ntotal;
  pool->pp_nslabs++;
}

static void destroy_slab(pool_t *pool, pool_slab_t *slab) {
  klog("destroy_slab: pool = %p, slab = %p", pool, slab);

  for (int i = 0; i < slab->ph_ntotal; i++) {
    pool_item_t *curr_pi = slab_item_at(slab, i);
    if (pool->pp_dtor)
      pool->pp_dtor(curr_pi->pi_data);
  }

  slab->ph_state = DEAD;
  kmem_free(slab, slab->ph_size);
}

static void *alloc_item(pool_slab_t *slab) {
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

static void destroy_slab_list(pool_t *pool, pool_slabs_t *slabs) {
  pool_slab_t *it, *next;

  LIST_FOREACH_SAFE (it, slabs, ph_slablist, next) {
    LIST_REMOVE(it, ph_slablist);
    destroy_slab(pool, it);
  }
}

void *pool_alloc(pool_t *pool, unsigned flags) {
  debug("pool_alloc: pool=%p", pool);

  SCOPED_MTX_LOCK(&pool->pp_mtx);
  assert(pool->pp_state == ALIVE);

  pool_slab_t *slab;
  if (pool->pp_nitems) {
    pool_slabs_t *slabs = LIST_EMPTY(&pool->pp_part_slabs)
                            ? &pool->pp_empty_slabs
                            : &pool->pp_part_slabs;
    slab = LIST_FIRST(slabs);
  } else {
    assert(pool != P_POOL); /* Master pool must use only static memory! */
    slab = kmem_alloc(PAGESIZE, flags);
    assert(slab != NULL);
    add_slab(pool, slab, PAGESIZE);
  }

  LIST_REMOVE(slab, ph_slablist);

  void *p = alloc_item(slab);
  pool->pp_nitems--;
  pool_slabs_t *slabs = (slab->ph_nused < slab->ph_ntotal)
                          ? &pool->pp_part_slabs
                          : &pool->pp_full_slabs;
  LIST_INSERT_HEAD(slabs, slab, ph_slablist);

  /* Create redzone after the item */
  kasan_mark(p, pool->pp_itemsize, pool->pp_itemsize + pool->pp_redzsize,
             KASAN_CODE_POOL_OVERFLOW);
  /* XXX: Modify code below when pp_ctor & pp_dtor are reenabled */
  if (flags & M_ZERO)
    bzero(p, pool->pp_itemsize);

  return p;
}

/* TODO: destroy empty slabs when their number reaches a certain threshold
 * (maybe leave one) */
static void _pool_free(pool_t *pool, void *ptr) {
  assert(mtx_owned(&pool->pp_mtx));

  debug("pool_free: pool = %p, ptr = %p", pool, ptr);
  assert(pool->pp_state == ALIVE);

  pool_item_t *pi = ptr - sizeof(pool_item_t);
  assert(pi->pi_canary == PI_MAGIC);
  pool_slab_t *slab = pi->pi_slab;
  assert(slab->ph_state == ALIVE);

  unsigned index = slab_index_of(slab, pi);
  bitstr_t *bitmap = slab->ph_bitmap;

  if (!bit_test(bitmap, index))
    panic("Double free detected in '%s' pool at %p!", pool->pp_desc, ptr);

  bit_clear(bitmap, index);
  LIST_REMOVE(slab, ph_slablist);
  slab->ph_nused--;
  pool->pp_nitems++;
  pool_slabs_t *slabs =
    slab->ph_nused ? &pool->pp_part_slabs : &pool->pp_empty_slabs;
  LIST_INSERT_HEAD(slabs, slab, ph_slablist);

  debug("pool_free: freed item %p at slab %p, index %d", ptr, slab, index);
}

void pool_free(pool_t *pool, void *ptr) {
  SCOPED_MTX_LOCK(&pool->pp_mtx);

  kasan_mark_invalid(ptr, pool->pp_itemsize + pool->pp_redzsize,
                     KASAN_CODE_POOL_USE_AFTER_FREE);
  kasan_quar_additem(&pool->pp_quarantine, ptr);
#ifndef KASAN
  /* Without KASAN, call regular free method */
  _pool_free(pool, ptr);
#endif /* !KASAN */
}

static void pool_ctor(pool_t *pool) {
  LIST_INIT(&pool->pp_empty_slabs);
  LIST_INIT(&pool->pp_full_slabs);
  LIST_INIT(&pool->pp_part_slabs);
  mtx_init(&pool->pp_mtx, 0);
  pool->pp_align = PI_ALIGNMENT;
  pool->pp_state = INITME;
}

static void pool_dtor(pool_t *pool) {
  /* Turn off preemption while marking the pool dead.
   *
   * There is no way to use pool's mutex here because it could already got
   * deallocated and we have no method of marking dead mutexes. */
  WITH_NO_PREEMPTION {
    assert(pool->pp_state == ALIVE);
    pool->pp_state = DEAD;
  }

  destroy_slab_list(pool, &pool->pp_empty_slabs);
  destroy_slab_list(pool, &pool->pp_part_slabs);
  destroy_slab_list(pool, &pool->pp_full_slabs);

  klog("destroyed pool '%s' at %p", pool->pp_desc, pool);
}

static void pool_init(pool_t *pool, const char *desc, size_t size,
                      pool_ctor_t ctor, pool_dtor_t dtor) {
  pool_ctor(pool);
  pool->pp_desc = desc;
  pool->pp_ctor = ctor;
  pool->pp_dtor = dtor;
  pool->pp_state = ALIVE;
#ifdef KASAN
  /* the alignment is within the redzone */
  pool->pp_itemsize = size;
  pool->pp_redzsize = align_size(size) - size + KASAN_POOL_REDZONE_SIZE;
#else
  /* no redzone, we have to align the size itself */
  pool->pp_itemsize = align_size(size);
#endif /* !KASAN */
  kasan_quar_init(&pool->pp_quarantine, pool, &pool->pp_mtx,
                  (quar_free_t)_pool_free);
  klog("initialized '%s' pool at %p (item size = %d)", pool->pp_desc, pool,
       pool->pp_itemsize);
}

void pool_bootstrap(void) {
  pool_init(P_POOL, "master pool", sizeof(pool_t), NULL, NULL);
  pool_add_page(P_POOL, P_POOL_BOOTPAGE, sizeof(P_POOL_BOOTPAGE));
  INVOKE_CTORS(pool_ctor_table);
}

void pool_add_page(pool_t *pool, void *page, size_t size) {
  assert(is_aligned(size, PAGESIZE));
  SCOPED_MTX_LOCK(&pool->pp_mtx);
  add_slab(pool, page, size);
}

pool_t *pool_create(const char *desc, size_t size) {
  pool_t *pool = pool_alloc(P_POOL, M_ZERO);
  pool_init(pool, desc, size, NULL, NULL);
  return pool;
}

void pool_destroy(pool_t *pool) {
  kasan_quar_releaseall(&pool->pp_quarantine);
  pool_dtor(pool);
  pool_free(P_POOL, pool);
}
