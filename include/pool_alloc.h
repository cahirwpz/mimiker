#include <queue.h>
#include <vm.h>
#include <bitstring.h>

typedef LIST_HEAD(, pool_slab) pool_slab_list_t;

typedef struct pool {
  pool_slab_list_t pp_empty_slabs;
  pool_slab_list_t pp_full_slabs;
  pool_slab_list_t pp_part_slabs; /* Partially-allocated pages */
  void (*pp_constructor)(void *, size_t);
  void (*pp_destructor)(void *, size_t);
  size_t pp_itemsize; /* Size of item */
  // size_t           pp_align;       /* Requested alignment, must be 2^n */
  size_t pp_nslabs; /* # of slabs allocated */
  size_t pp_nitems; /* number of available items in pool */
} pool_t;

void pool_init(pool_t *buf, size_t size, void (*constructor)(void *, size_t),
               void (*destructor)(void *, size_t));
void pool_destroy(pool_t *pool);
void *pool_alloc(pool_t *pool, uint32_t flags);
void pool_free(pool_t *pool, void *ptr);
void pool_default_destructor(void *buf, size_t size);
