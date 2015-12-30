#ifndef __BITOPS_H__
#define __BITOPS_H__

#define   SET_BIT(n, b) (n |=  (1 << b) )
#define CLEAR_BIT(n, b) (n &= ~(1 << b) )
#define   GET_BIT(n, b) ((n & (1<<b)) != 0)

#endif // __BITOPS_H__
