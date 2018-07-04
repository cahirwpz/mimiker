#ifndef _SYS_POOL_H_
#define _SYS_POOL_H_

#include <common.h>
#include <linker_set.h>

typedef struct pool pool_t;

void pool_bootstrap(void);

#define PF_ZERO 1 /* clear allocated block */

pool_t *pool_create(const char *desc, size_t size);
void pool_destroy(pool_t *pool);
void *pool_alloc(pool_t *pool, unsigned flags);
void pool_free(pool_t *pool, void *ptr);

#define POOL_DEFINE(name, ...)                                                 \
  struct pool *name;                                                           \
  static void __ctor_##name(void) {                                            \
    name = pool_create(__VA_ARGS__);                                           \
  }                                                                            \
  SET_ENTRY(pool_ctor_table, __ctor_##name);

#endif /* !_SYS_POOL_H_ */
