#ifndef _SYS_SYSTM_H_
#define _SYS_SYSTM_H_

#include <sys/cdefs.h>

int copystr(const void *restrict kfaddr, void *restrict kdaddr, size_t len,
            size_t *restrict lencopied) __nonnull(1) __nonnull(2);
int copyinstr(const void *restrict udaddr, void *restrict kaddr, size_t len,
              size_t *restrict lencopied) __nonnull(1) __nonnull(2);
int copyin(const void *restrict udaddr, void *restrict kaddr, size_t len)
  __nonnull(1) __nonnull(2);
int copyout(const void *restrict kaddr, void *restrict udaddr, size_t len)
  __nonnull(1) __nonnull(2);

#define copyin_s(udaddr, _what) copyin((udaddr), &(_what), sizeof(_what))
#define copyout_s(_what, udaddr) copyout(&(_what), (udaddr), sizeof(_what))

#endif /* !_SYS_SYSTM_H_ */
