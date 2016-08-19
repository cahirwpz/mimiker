#ifndef __MUTEX_SLEEPQ_H__
#define __MUTEX_SLEEPQ_H__

#include <common.h>
#include <mips.h>

typedef uintptr_t mtx_sleepq_t;

void mtx_sleepq_init(mtx_sleepq_t *mtx);
void mtx_sleepq_lock(mtx_sleepq_t *mtx);
void mtx_sleepq_unlock(mtx_sleepq_t *mtx);

#endif /* __MUTEX_SLEEPQ_H__ */