#ifndef _SYS_POOL_H_
#define _SYS_POOL_H_

#include <bitstring.h>
#include <sys/cdefs.h>
#include <sys/linker_set.h>
#include <sys/kasan.h>
#include <sys/kmem_flags.h>
#include <sys/mutex.h>
#include <sys/queue.h>

/*! \file pool.h
 *
 * Pooled allocator manages fixed-size object. Implementation is based on idea
 * of the slab allocator, however object caching facility is not available
 * (though the actual implementation is capable of supporting it).
 *
 * Pooled allocator idea is loosely based on NetBSD's pool(9).
 */

typedef LIST_HEAD(, slab) slab_list_t;

typedef struct pool {
  TAILQ_ENTRY(pool) pp_link;
  mtx_t pp_mtx;
  const char *pp_desc;
  slab_list_t pp_empty_slabs;
  slab_list_t pp_full_slabs;
  slab_list_t pp_part_slabs; /* partially allocated slabs */
  size_t pp_itemsize;        /* size of item */
  size_t pp_alignment;       /* alignment of allocated items */
  size_t pp_slabsize;        /* size of a single slab */
#if KASAN
  size_t pp_redzone; /* size of redzone after each item */
  quar_t pp_quarantine;
#endif
  /* statistics */
  size_t pp_npages;   /* number of allocated pages (in bytes) */
  size_t pp_nused;    /* number of used items in all slabs */
  size_t pp_nmaxused; /* peak number of used items in all slabs */
  size_t pp_ntotal;   /* total number of items in all slabs */
} pool_t;

typedef struct slab {
  LIST_ENTRY(slab) ph_link; /* pool slab list */
  pool_t *ph_pool;          /* pool handle */
  uint16_t ph_nused;        /* # of items in use */
  uint16_t ph_ntotal;       /* total number of items */
  size_t ph_size;           /* size of memory allocated for the slab */
  size_t ph_itemsize;       /* total size of item (with header and redzone) */
  void *ph_items;           /* ptr to array of items after bitmap */
  bitstr_t ph_bitmap[0];
} slab_t;

/*! \brief Called during kernel initialization. */
void init_pool(void);

/*! \brief Pool constructor parameters. */
typedef struct pool_init {
  const char *desc;
  size_t size;
  size_t alignment;
  size_t slabsize;
} pool_init_t;

/*! \brief Creates a pool of objects of given size. */
pool_t *_pool_create(pool_init_t *args);

#define pool_create(...) _pool_create(&(pool_init_t){__VA_ARGS__})

/*! \brief Initializes specified pool. */
void _pool_init(pool_t *pool, pool_init_t *args);

#define pool_init(pool, ...) _pool_init((pool), &(pool_init_t){__VA_ARGS__})

/*! \brief Adds a signle slab to the pool.
 *
 * `size` must be >= `pool->pp_slabsize` and divisible by `PAGESIZE`.
 *
 * \note Use only during memory system bootstrap!
 */
void pool_add_page(pool_t *pool, void *page, size_t size);

/*! \brief Frees all memory associated with the pool
 *
 * \warning Do not call this function on pool with live objects! */
void pool_destroy(pool_t *pool);

/*! \brief Allocate an object from the pool.
 *
 * \note The pool may grow in page size units. */
void *pool_alloc(pool_t *pool, kmem_flags_t flags) __warn_unused;

/*! \brief Release an object that belongs to the pool. */
void pool_free(pool_t *pool, void *ptr);

/*! \brief Define a pool that will be initialized during system startup. */
#define POOL_DEFINE(NAME, ...)                                                 \
  pool_t NAME[1];                                                              \
  static void __ctor_##NAME(void) {                                            \
    pool_init(NAME, __VA_ARGS__);                                              \
  }                                                                            \
  SET_ENTRY(pool_ctor_table, __ctor_##NAME);

#endif /* !_SYS_POOL_H_ */
