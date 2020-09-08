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

typedef LIST_HEAD(, slab) slab_list_t;

typedef struct pool {
  TAILQ_ENTRY(pool) pp_link;
  mtx_t pp_mtx;
  const char *pp_desc;
  slab_list_t pp_empty_slabs;
  slab_list_t pp_full_slabs;
  slab_list_t pp_part_slabs; /* partially allocated slabs */
  pool_ctor_t pp_ctor;
  pool_dtor_t pp_dtor;
  size_t pp_itemsize; /* size of item */
#if KASAN
  size_t pp_redzsize; /* size of redzone after each item */
  quar_t pp_quarantine;
#endif
} pool_t;

static TAILQ_HEAD(, pool) pool_list = TAILQ_HEAD_INITIALIZER(pool_list);
static mtx_t *pool_list_lock = &MTX_INITIALIZER(0);

typedef struct slab {
  LIST_ENTRY(slab) ph_link; /* pool slab list */
  uint16_t ph_nused;        /* # of items in use */
  uint16_t ph_ntotal;       /* total number of chunks */
  size_t ph_size;           /* size of memory allocated for the slab */
  size_t ph_itemsize;       /* total size of item (with header and redzone) */
  void *ph_items;           /* ptr to array of items after bitmap */
  bitstr_t ph_bitmap[0];
} slab_t;

typedef struct pool_item {
  uint32_t pi_canary; /* PI_MAGIC by default, if modified then memory
                       * corruption has occured */
  slab_t *pi_slab;    /* pointer to slab containing this item */
  unsigned long pi_data[0] __aligned(PI_ALIGNMENT);
} pool_item_t;

/* Pool of pool_t objects. */
static pool_t P_POOL[1];
static alignas(PAGESIZE) uint8_t P_POOL_BOOTPAGE[PAGESIZE * 2];

static pool_item_t *slab_item_at(slab_t *slab, unsigned i) {
  return (pool_item_t *)(slab->ph_items + i * slab->ph_itemsize);
}

static unsigned slab_index_of(slab_t *slab, pool_item_t *item) {
  return ((intptr_t)item - (intptr_t)slab->ph_items) / slab->ph_itemsize;
}

static void add_slab(pool_t *pool, slab_t *slab, size_t slabsize) {
  assert(mtx_owned(&pool->pp_mtx));

  klog("add slab at %p to '%s' pool", slab, pool->pp_desc);

  slab->ph_nused = 0;
  slab->ph_size = slabsize;
  slab->ph_itemsize = pool->pp_itemsize + sizeof(pool_item_t);
#if KASAN
  slab->ph_itemsize += pool->pp_redzsize;
#endif /* !KASAN */

  /*
   * Now we need to calculate maximum possible number of items of given `size`
   * in slab that occupies one page, taking into account space taken by:
   *  - items: ntotal * (sizeof(pool_item_t) + size),
   *  - slab + bitmap: sizeof(slab_t) + bitstr_size(ntotal)
   * With:
   *  - usable = slabsize - sizeof(slab_t)
   *  - itemsize = sizeof(pool_item_t) + size;
   * ... inequation looks as follow:
   * (1) ntotal * itemsize + (ntotal + 7) / 8 <= usable
   * (2) ntotal * 8 * itemsize + ntotal + 7 <= usable * 8
   * (3) ntotal * (8 * itemsize + 1) <= usable * 8 + 7
   * (4) ntotal <= (usable * 8 + 7) / (8 * itemisize + 1)
   */
  size_t usable = slabsize - sizeof(slab_t);
  slab->ph_ntotal = (usable * 8 + 7) / (8 * slab->ph_itemsize + 1);
  slab->ph_nused = 0;

  size_t header = sizeof(slab_t) + bitstr_size(slab->ph_ntotal);
  slab->ph_items = (void *)slab + align(header, PI_ALIGNMENT);
  memset(slab->ph_bitmap, 0, bitstr_size(slab->ph_ntotal));

  for (int i = 0; i < slab->ph_ntotal; i++) {
    pool_item_t *pi = slab_item_at(slab, i);
    pi->pi_slab = slab;
    pi->pi_canary = PI_MAGIC;
    if (pool->pp_ctor)
      pool->pp_ctor(pi->pi_data);
  }

  LIST_INSERT_HEAD(&pool->pp_empty_slabs, slab, ph_link);
}

static void destroy_slab(pool_t *pool, slab_t *slab) {
  klog("destroy_slab: pool = %p, slab = %p", pool, slab);

  for (int i = 0; i < slab->ph_ntotal; i++) {
    pool_item_t *curr_pi = slab_item_at(slab, i);
    if (pool->pp_dtor)
      pool->pp_dtor(curr_pi->pi_data);
  }

  kmem_free(slab, slab->ph_size);
}

static void *alloc_item(slab_t *slab) {
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

static void destroy_slab_list(pool_t *pool, slab_list_t *slabs) {
  slab_t *it, *next;

  LIST_FOREACH_SAFE (it, slabs, ph_link, next) {
    LIST_REMOVE(it, ph_link);
    destroy_slab(pool, it);
  }
}

void *pool_alloc(pool_t *pool, unsigned flags) {
  void *ptr;

  debug("pool_alloc: pool=%p", pool);

  WITH_MTX_LOCK(&pool->pp_mtx) {
    slab_t *slab;

    if (!(slab = LIST_FIRST(&pool->pp_part_slabs))) {
      if (!(slab = LIST_FIRST(&pool->pp_empty_slabs))) {
        assert(pool != P_POOL); /* Master pool must use only static memory! */
        slab = kmem_alloc(PAGESIZE, flags);
        assert(slab != NULL);
        add_slab(pool, slab, PAGESIZE);
      }
    }

    if (slab->ph_nused == 0) {
      /* We're going to allocate from empty slab
       * -> move it to the list of non-empty slabs. */
      LIST_REMOVE(slab, ph_link);
      LIST_INSERT_HEAD(&pool->pp_part_slabs, slab, ph_link);
    }

    ptr = alloc_item(slab);

    if (slab->ph_nused == slab->ph_ntotal) {
      /* We've allocated last item from non-empty slab
       * -> move it to the list of full slabs. */
      LIST_REMOVE(slab, ph_link);
      LIST_INSERT_HEAD(&pool->pp_full_slabs, slab, ph_link);
    }
  }

  /* Create redzone after the item */
  kasan_mark(ptr, pool->pp_itemsize, pool->pp_itemsize + pool->pp_redzsize,
             KASAN_CODE_POOL_OVERFLOW);
  /* XXX: Modify code below when pp_ctor & pp_dtor are reenabled */
  if (flags & M_ZERO)
    bzero(ptr, pool->pp_itemsize);

  return ptr;
}

/* TODO: destroy empty slabs when their number reaches a certain threshold
 * (maybe leave one) */
static void _pool_free(pool_t *pool, void *ptr) {
  assert(mtx_owned(&pool->pp_mtx));

  debug("pool_free: pool = %p, ptr = %p", pool, ptr);

  pool_item_t *pi = ptr - sizeof(pool_item_t);
  assert(pi->pi_canary == PI_MAGIC);
  slab_t *slab = pi->pi_slab;

  unsigned index = slab_index_of(slab, pi);
  bitstr_t *bitmap = slab->ph_bitmap;

  if (!bit_test(bitmap, index))
    panic("Double free detected in '%s' pool at %p!", pool->pp_desc, ptr);

  bit_clear(bitmap, index);
  LIST_REMOVE(slab, ph_link);
  slab->ph_nused--;
  slab_list_t *slabs =
    slab->ph_nused ? &pool->pp_part_slabs : &pool->pp_empty_slabs;
  LIST_INSERT_HEAD(slabs, slab, ph_link);

  debug("pool_free: freed item %p at slab %p, index %d", ptr, slab, index);
}

void pool_free(pool_t *pool, void *ptr) {
  SCOPED_MTX_LOCK(&pool->pp_mtx);

  kasan_mark_invalid(ptr, pool->pp_itemsize + pool->pp_redzsize,
                     KASAN_CODE_POOL_FREED);
  kasan_quar_additem(&pool->pp_quarantine, ptr);
#if !KASAN
  /* Without KASAN, call regular free method */
  _pool_free(pool, ptr);
#endif /* !KASAN */
}

static void pool_ctor(pool_t *pool) {
  LIST_INIT(&pool->pp_empty_slabs);
  LIST_INIT(&pool->pp_full_slabs);
  LIST_INIT(&pool->pp_part_slabs);
  mtx_init(&pool->pp_mtx, 0);
}

static void pool_dtor(pool_t *pool) {
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
#if KASAN
  /* the alignment is within the redzone */
  pool->pp_itemsize = size;
  pool->pp_redzsize =
    align(size, PI_ALIGNMENT) - size + KASAN_POOL_REDZONE_SIZE;
#else /* !KASAN */
  /* no redzone, we have to align the size itself */
  pool->pp_itemsize = align(size, PI_ALIGNMENT);
#endif
  kasan_quar_init(&pool->pp_quarantine, pool, (quar_free_t)_pool_free);
  klog("initialized '%s' pool at %p (item size = %d)", pool->pp_desc, pool,
       pool->pp_itemsize);
  WITH_MTX_LOCK (pool_list_lock)
    TAILQ_INSERT_TAIL(&pool_list, pool, pp_link);
}

void init_pool(void) {
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
  WITH_MTX_LOCK (pool_list_lock)
    TAILQ_REMOVE(&pool_list, pool, pp_link);
  WITH_MTX_LOCK (&pool->pp_mtx)
    /* Lock needed as the quarantine may call _pool_free! */
    kasan_quar_releaseall(&pool->pp_quarantine);
  pool_dtor(pool);
  pool_free(P_POOL, pool);
}
