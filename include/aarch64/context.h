#ifndef _AARCH64_CONTEXT_H_
#define _AARCH64_CONTEXT_H_

#ifndef _MACHDEP
#error "Do not use this header file outside kernel machine dependent code!"
#endif

typedef struct ctx {
  register_t x[30];
  register_t sp, fp, lr;
  register_t pc, spsr;
} ctx_t;

#endif /* !_AARCH64_CONTEXT_H_ */
