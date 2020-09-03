#ifndef _AARCH64_EXCEPTION_H_
#define _AARCH64_EXCEPTION_H_

#ifndef _MACHDEP
#error "Do not use this header file outside kernel machine dependent code!"
#endif

#include <stdint.h>

typedef struct exc_frame {
  uint64_t sp;
  uint64_t lr;
  uint64_t far; /* fault address register */
  uint64_t pc; /* exception link register */
  uint32_t spsr; /* saved program status register */
  uint32_t esr; /* exception syndrome register */
  uint64_t x[30];
} exc_frame_t;

/* TODO exception frame for userspace */
#define cpu_exc_frame_t exc_frame_t

#endif /* !_AARCH64_EXCEPTION_H_ */
