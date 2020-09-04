#ifndef _AARCH64_CONTEXT_H_
#define _AARCH64_CONTEXT_H_

#ifndef _MACHDEP
#error "Do not use this header file outside kernel machine dependent code!"
#endif

#include <sys/context.h>
#include <sys/types.h>
#include <stdbool.h>

typedef struct ctx {
  uint64_t sp;
  uint64_t lr;
  uint64_t far;
  uint64_t elr;
  uint32_t spsr;
  uint32_t esr;
  uint64_t x[30];
} ctx_t;

#endif /* !_AARCH64_CONTEXT_H_ */
