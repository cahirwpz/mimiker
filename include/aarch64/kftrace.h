#ifndef __AARCH64_KFTRACE_H__
#define __AARCH64_KFTRACE_H__

#include <aarch64/armreg.h>

#define KFT_EVENT_MAX 0x100000
#define kft_get_time() READ_SPECIALREG(cntpct_el0)

#endif /* __AARCH64_KFTRACE_H__ */
