#ifndef _SYS_MALLOC_H_
#define _SYS_MALLOC_H_

#include <sys/types.h>
#include <sys/linker_set.h>
#include <sys/kmem_flags.h>
#include <sys/mutex.h>

/*
 * General purpose kernel memory allocator.
 */

typedef struct kmalloc_pool {
  const char *desc; /* Printable type name. */
  mtx_t lock;       /* Guards fields below. */
  size_t nrequests; /* Number of allocation requests. */
  size_t active;    /* Numer of active blocks. */
  size_t used;      /* Bytes currently used. */
  size_t maxused;   /* Peak usage of memory. */
} kmalloc_pool_t;

/* Defines a local pool of memory for use by a subsystem. */
#define KMALLOC_DEFINE(NAME, DESC)                                             \
  kmalloc_pool_t NAME[1] = {{                                                  \
    .lock = MTX_INITIALIZER(kmalloc_pool, MTX_SPIN),                           \
    .desc = (DESC),                                                            \
  }};                                                                          \
  SET_ENTRY(kmalloc_pool, NAME)

#define KMALLOC_DECLARE(NAME) extern kmalloc_pool_t NAME[1];

/*! \brief Called during kernel initialization. */
void init_kmalloc(void);

void *kmalloc(kmalloc_pool_t *mp, size_t size,
              kmem_flags_t flags) __warn_unused;
void kfree(kmalloc_pool_t *mp, void *addr);
char *kstrndup(kmalloc_pool_t *mp, const char *s, size_t maxlen);

/*! \brief M_TEMP delivers storage for short lived temporary objects. */
KMALLOC_DECLARE(M_TEMP);
/*! \brief M_STR delivers storage for NUL-terminated strings. */
KMALLOC_DECLARE(M_STR);

#endif /* !_SYS_MALLOC_H_ */
