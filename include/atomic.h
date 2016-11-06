#include <stdc.h>
int atomic_cmp_exchange(__volatile uint32_t *p, uint32_t cmpval, uint32_t newval);
void atomic_store(__volatile uint32_t *p, uint32_t val);
