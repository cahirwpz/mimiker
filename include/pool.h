#ifndef _SYS_POOL_H_
#define _SYS_POOL_H_

#include <common.h>
#include <linker_set.h>

/*! \file pool.h
 *
 * Pooled allocator manages fixed-size object. Implementation is based on idea
 * of the slab allocator, however object caching facility is not available
 * (though the actual implementation is capable of supporting it).
 *
 * Pooled allocator idea is loosely based on NetBSD's pool(9).
 */

typedef struct pool pool_t;

void pool_bootstrap(void);

/* Flags that can be passed to `pool_alloc`. */
#define PF_ZERO 1 /* clear allocated block */

/*! \brief Creates a pool of objects of given size. */
pool_t *pool_create(const char *desc, size_t size);

/*! \brief Frees all memory associated with the pool
 *
 * \warning Do not call this function on pool with live objects! */
void pool_destroy(pool_t *pool);

/*! \brief Allocate an object from the pool.
 *
 * \note The pool may grow in page size units. */
void *pool_alloc(pool_t *pool, unsigned flags);

/*! \brief Release an object that belongs to the pool. */
void pool_free(pool_t *pool, void *ptr);

/*! \brief Define a pool that will be initialized during system startup. */
#define POOL_DEFINE(name, ...)                                                 \
  struct pool *name;                                                           \
  static void __ctor_##name(void) {                                            \
    name = pool_create(__VA_ARGS__);                                           \
  }                                                                            \
  SET_ENTRY(pool_ctor_table, __ctor_##name);

#endif /* !_SYS_POOL_H_ */
