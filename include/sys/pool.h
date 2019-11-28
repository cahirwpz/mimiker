#ifndef _SYS_POOL_H_
#define _SYS_POOL_H_

#include <sys/cdefs.h>
#include <sys/linker_set.h>
#include <sys/kmem_flags.h>

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

/*! \brief Creates a pool of objects of given size. */
pool_t *pool_create(const char *desc, size_t size);

/*! \brief Adds a slab of one page to the pool.
 *
 * \note Use only during memory system bootstrap!
 */
void pool_add_page(pool_t *pool, void *page);

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
#define POOL_DEFINE(name, ...)                                                 \
  struct pool *name;                                                           \
  static void __ctor_##name(void) {                                            \
    name = pool_create(__VA_ARGS__);                                           \
  }                                                                            \
  SET_ENTRY(pool_ctor_table, __ctor_##name);

#endif /* !_SYS_POOL_H_ */
