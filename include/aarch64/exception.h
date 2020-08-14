#ifndef _AARCH64_EXCEPTION_H_
#define _AARCH64_EXCEPTION_H_

#ifndef _MACHDEP
#error "Do not use this header file outside kernel machine dependent code!"
#endif

#include <stdint.h>

typedef struct exc_frame {
  uint64_t sp;
  uint64_t lr;
  uint64_t far;
  uint64_t elr;
  uint32_t spsr;
  uint32_t esr;
  uint64_t x[30];
} exc_frame_t;

#endif /* !_AARCH64_EXCEPTION_H_ */
