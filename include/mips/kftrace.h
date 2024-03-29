#ifndef __MIPS_KFTRACE_H__
#define __MIPS_KFTRACE_H__

#define _MACHDEP

#include <mips/m32c0.h>

#define KFT_EVENT_MAX 0x4000
#define kft_get_time() mips32_getcount()

#undef _MACHDEP

#endif /* __MIPS_KFTRACE_H__ */
