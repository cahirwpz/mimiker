#ifndef _SYS_POOL_H_
#define _SYS_POOL_H_

#include <queue.h>
#include <mutex.h>

typedef LIST_HEAD(, pool_slab) pool_slab_list_t;
typedef void (*pool_ctor_t)(void *);
typedef void (*pool_dtor_t)(void *);

typedef struct pool {
  mtx_t pp_mtx;
  uint32_t pp_state;
  pool_slab_list_t pp_empty_slabs;
  pool_slab_list_t pp_full_slabs;
  pool_slab_list_t pp_part_slabs; /* partially allocated slabs */
  pool_ctor_t pp_ctor;
  pool_dtor_t pp_dtor;
  size_t pp_itemsize; /* size of item */
  size_t pp_align;    /* (ignored) requested alignment, must be 2^n */
  size_t pp_nslabs;   /* # of slabs allocated */
  size_t pp_nitems;   /* number of available items in pool */
} pool_t;

void pool_init(pool_t *pool, size_t size, pool_ctor_t ctor, pool_dtor_t dtor);
void pool_destroy(pool_t *pool);
void *pool_alloc(pool_t *pool, unsigned flags);
void pool_free(pool_t *pool, void *ptr);

#endif
