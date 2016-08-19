#ifndef __MUTEX_YIELD_H__
#define __MUTEX_YIELD_H__

#include <common.h>
#include <mips.h>

typedef volatile uintptr_t mtx_yield_t;

void mtx_yield_init(mtx_yield_t *mtx);
void mtx_yield_lock(mtx_yield_t *mtx);
void mtx_yield_unlock(mtx_yield_t *mtx);

#endif /* __MUTEX_YIELD_H__ */