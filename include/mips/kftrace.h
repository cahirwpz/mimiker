#ifndef __MIPS_KFTRACE_H__
#define __MIPS_KFTRACE_H__

#define _MACHDEP

#include <mips/m32c0.h>

#define kft_get_time() mips32_getcount()

#undef _MACHDEP

#endif /* __MIPS_KFTRACE_H__ */
