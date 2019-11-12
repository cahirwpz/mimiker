#ifndef _SYS_MALLOC_H_
#define _SYS_MALLOC_H_

#include <sys/types.h>
#include <sys/linker_set.h>
#include <machine/vm_param.h>

/*
 * General purpose kernel memory allocator.
 */

typedef struct kmem_pool kmem_pool_t;

/* Defines a local pool of memory for use by a subsystem. */
#define MALLOC_DEFINE(name, ...)                                               \
  struct kmem_pool *name;                                                      \
  static void __ctor_##name(void) {                                            \
    name = kmem_create(__VA_ARGS__);                                           \
  }                                                                            \
  SET_ENTRY(kmem_ctor_table, __ctor_##name);

#define MALLOC_DECLARE(name) extern kmem_pool_t *name;

/* Flags to malloc */
#define M_WAITOK 0x0000 /* always returns memory block, but can sleep */
#define M_NOWAIT 0x0001 /* may return NULL, but cannot sleep */
#define M_ZERO 0x0002   /* clear allocated block */

void kmem_bootstrap(void);
kmem_pool_t *kmem_create(const char *desc, size_t maxsize);
int kmem_reserve(kmem_pool_t *mp, size_t size);
void kmem_dump(kmem_pool_t *mp);
void kmem_destroy(kmem_pool_t *mp);

void *kmalloc(kmem_pool_t *mp, size_t size, unsigned flags) __warn_unused;
void kfree(kmem_pool_t *mp, void *addr);
char *kstrndup(kmem_pool_t *mp, const char *s, size_t maxlen);

/*! \brief M_TEMP delivers storage for short lived temporary objects. */
MALLOC_DECLARE(M_TEMP);
/*! \brief M_STR delivers storage for NUL-terminated strings. */
MALLOC_DECLARE(M_STR);

#endif /* !_SYS_MALLOC_H_ */
