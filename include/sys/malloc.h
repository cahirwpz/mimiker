#ifndef _SYS_MALLOC_H_
#define _SYS_MALLOC_H_

#include <sys/types.h>
#include <sys/linker_set.h>
#include <sys/kmem_flags.h>
#include <machine/vm_param.h>

/*
 * General purpose kernel memory allocator.
 */

typedef struct kmalloc_pool {
  const char *mp_desc; /* Printable type name. */
} kmalloc_pool_t;

/* Defines a local pool of memory for use by a subsystem. */
#define KMALLOC_DEFINE(name, desc)                                             \
  kmalloc_pool_t *name = &(kmalloc_pool_t) {                                   \
    .mp_desc = (desc)                                                          \
  }

#define KMALLOC_DECLARE(name) extern kmalloc_pool_t *name;

/*! \brief Called during kernel initialization. */
void init_kmalloc(void);

void *kmalloc(kmalloc_pool_t *mp, size_t size,
              kmem_flags_t flags) __warn_unused;
void kfree(kmalloc_pool_t *mp, void *addr);
char *kstrndup(kmalloc_pool_t *mp, const char *s, size_t maxlen);

void kmcheck(int verbose);

/*! \brief M_TEMP delivers storage for short lived temporary objects. */
KMALLOC_DECLARE(M_TEMP);
/*! \brief M_STR delivers storage for NUL-terminated strings. */
KMALLOC_DECLARE(M_STR);

#endif /* !_SYS_MALLOC_H_ */
