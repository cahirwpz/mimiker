#ifndef _MIPS_CONTEXT_H_
#define _MIPS_CONTEXT_H_

#ifndef _MACHDEP
#error "Do not use this header file outside kernel machine dependent code!"
#endif

#include <sys/types.h>

typedef struct ctx {
  register_t s0, s1, s2, s3, s4, s5, s6, s7;
  register_t v0, gp, sp, fp, ra;
  register_t pc, sr;
} ctx_t;

#endif /* !_MIPS_CONTEXT_H_ */
