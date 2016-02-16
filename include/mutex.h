#ifndef __SYS_MUTEX_H__
#define __SYS_MUTEX_H__

#include <common.h>

typedef volatile uintptr_t mtx_t;

#define MTX_INITIALIZER 0

#define mtx_lock(m) __extension__ ({ \
  m = _mips_intdisable();            \
})

#define mtx_unlock(m) __extension__ ({ \
  _mips_intrestore(m);                 \
})

#endif /* __SYS_MUTEX_H__ */
