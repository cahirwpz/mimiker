#include <stdlib.h>
#include "queue.h"

typedef struct pool_slab {
  LIST_ENTRY(pool_slab) ph_slablist; /* pool slab list */
  uint16_t ph_nused;                 /* # of items in use */
  uint16_t ph_nfree;  /* # of free (and available) items, a bit redundant but
                         there would be padding abyway*/
  uint16_t ph_ntotal; /* total number of chunks*/
  uint16_t ph_start;  /* start offset in page */
  uint8_t ph_bitmap[0];
} pool_slab_t;

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