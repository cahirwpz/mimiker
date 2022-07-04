#ifndef _RISCV_TYPES_H_
#define _RISCV_TYPES_H_

#include <stdint.h>

#if __riscv_xlen == 64
typedef int64_t register_t;
#else
typedef int32_t register_t;
#endif

#ifdef __riscv_d
typedef int64_t fpregister_t;
#elif defined(__riscv_f)
typedef int32_t fpregister_t;
#else
typedef struct {
} fpregister_t;
#endif

#endif /* !_RISCV_TYPES_H_ */
