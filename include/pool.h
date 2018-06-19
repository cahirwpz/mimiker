#ifndef _SYS_POOL_H_
#define _SYS_POOL_H_

#include <common.h>

typedef struct pool *pool_t;

typedef void (*pool_ctor_t)(void *);
typedef void (*pool_dtor_t)(void *);

void pool_bootstrap(void);

pool_t pool_create(const char *desc, size_t size, pool_ctor_t ctor,
                   pool_dtor_t dtor);
void pool_destroy(pool_t pool);
void *pool_alloc(pool_t pool, unsigned flags);
void pool_free(pool_t pool, void *ptr);

#endif
