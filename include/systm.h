#ifndef _SYS_SYSTM_H_
#define _SYS_SYSTM_H_

#include "common.h"

int copystr(const void *restrict kfaddr, void *restrict kdaddr, size_t len,
            size_t *restrict lencopied) __nonnull(1) __nonnull(2);
int copyinstr(const void *restrict udaddr, void *restrict kaddr, size_t len,
              size_t *restrict lencopied) __nonnull(1) __nonnull(2);
int copyin(const void *restrict udaddr, void *restrict kaddr, size_t len)
  __nonnull(1) __nonnull(2);
int copyout(const void *restrict kaddr, void *restrict udaddr, size_t len)
  __nonnull(1) __nonnull(2);

/* fuword32() returns the data fetched or -1 on failure. */
int32_t fuword32(const void *ptr);
int suword32(void *ptr, int32_t word);

#endif /* !_SYS_SYSTM_H_ */
