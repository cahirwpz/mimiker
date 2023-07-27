#ifndef __KFTRACE_H__
#define __KFTRACE_H__

#include <aarch64/armreg.h>

/* Macro to read count from machine register in KFTRACE functions */
#define kft_get_time() READ_SPECIALREG(cntpct_el0)

#endif /* __KFTRACE_H__ */
