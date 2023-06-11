#ifndef __KFT_H__
#define __KFT_H__

#include <aarch64/armreg.h>

/* Macro to read count from machine register in KFT functions */
#define kft_get_time() READ_SPECIALREG(cntpct_el0)

#endif /* __KFT_H__ */
