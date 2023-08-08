#ifndef __RISCV_KFTRACE_H__
#define __RISCV_KFTRACE_H__

#include <riscv/cpufunc.h>

#define KFT_EVENT_MAX 1000000
#define kft_get_time() rdtime()

#endif /* __RISCV_KFTRACE_H__ */
