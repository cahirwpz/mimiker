#ifndef _AARCH64_CONTEXT_H_
#define _AARCH64_CONTEXT_H_

#ifndef _MACHDEP
#error "Do not use this header file outside kernel machine dependent code!"
#endif

#include <aarch64/mcontext.h>
#include <sys/context.h>
#include <sys/types.h>
#include <stdbool.h>

typedef struct ctx {
  __gregset_t __gregs;
} ctx_t;

#define _REG(ctx, n) ((ctx)->__gregs[_REG_##n])

#endif /* !_AARCH64_CONTEXT_H_ */
