#ifndef __ATOMIC_H__
#define __ATOMIC_H__

#include <stdc.h>

/* Performs atomic cmp_exchange:
 * if *p == cmpval:
 *  *p = newval
 *  return true
 * else:
 *  return false
 * */
int atomic_cmp_exchange(__volatile uint32_t *p, uint32_t cmpval,
                        uint32_t newval);

/* Performs atomic_store
 * *p = val
 * */
void atomic_store(__volatile uint32_t *p, uint32_t val);
#endif /* __ATOMIC_H__ */
