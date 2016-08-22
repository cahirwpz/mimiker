#ifndef __MUTEX_H__
#define __MUTEX_H__

#include <common.h>
#include <mips/cpu.h> 
#include <mips/mips.h>

typedef volatile uintptr_t mtx_t;

#define MTX_INITIALIZER 0

#define mtx_lock(m) __extension__ ({ \
  m = _mips_intdisable();            \
})

#define mtx_unlock(m) __extension__ ({ \
  _mips_intrestore(m);                 \
})

#endif /* __MUTEX_H__ */
