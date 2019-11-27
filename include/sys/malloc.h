#ifndef _SYS_MALLOC_H_
#define _SYS_MALLOC_H_

#include <sys/types.h>
#include <sys/linker_set.h>
#include <sys/kmem_flags.h>
#include <machine/vm_param.h>

/*
 * General purpose kernel memory allocator.
 */

typedef struct kmalloc_pool kmalloc_pool_t;

/* Defines a local pool of memory for use by a subsystem. */
#define KMALLOC_DEFINE(name, ...)                                              \
  struct kmalloc_pool *name;                                                   \
  static void __ctor_##name(void) {                                            \
    name = kmalloc_create(__VA_ARGS__);                                        \
  }                                                                            \
  SET_ENTRY(kmalloc_ctor_table, __ctor_##name);

#define KMALLOC_DECLARE(name) extern kmalloc_pool_t *name;

void kmalloc_bootstrap(void);
kmalloc_pool_t *kmalloc_create(const char *desc, size_t maxsize);
int kmalloc_reserve(kmalloc_pool_t *mp, size_t size);
void kmalloc_dump(kmalloc_pool_t *mp);
void kmalloc_destroy(kmalloc_pool_t *mp);

void *kmalloc(kmalloc_pool_t *mp, size_t size,
              kmem_flags_t flags) __warn_unused;
void kfree(kmalloc_pool_t *mp, void *addr);
char *kstrndup(kmalloc_pool_t *mp, const char *s, size_t maxlen);

/*! \brief M_TEMP delivers storage for short lived temporary objects. */
KMALLOC_DECLARE(M_TEMP);
/*! \brief M_STR delivers storage for NUL-terminated strings. */
KMALLOC_DECLARE(M_STR);

#endif /* !_SYS_MALLOC_H_ */
