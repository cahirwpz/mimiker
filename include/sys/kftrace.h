#ifndef __KFTRACE_H__
#define __KFTRACE_H__

#if KFTRACE
#include <machine/kftrace.h>
void init_kftrace(void);
#else
#define init_kftrace() __nothing
#endif

#endif /* __KFTRACE_H__ */
