#ifndef __ATOMIC_H__
#define __ATOMIC_H__

#include <stdc.h>

/*
 * Atomically compare the value stored at *p with cmpval and if the
 * two values are equal, update the value of *p with newval. Returns
 * zero if the compare failed, nonzero otherwise.
 * Pseudocode:
 * if *p == cmpval:
 *  *p = newval
 *  return true
 * else:
 *  return false
 */
int atomic_cmp_exchange(volatile uint32_t *p, uint32_t cmpval, uint32_t newval);

/* Atomically stores value stored in val on the memory address p.
 * Pseudocode:
 * *p = val
 * */
void atomic_store(volatile uint32_t *p, uint32_t val);

#endif /* __ATOMIC_H__ */
