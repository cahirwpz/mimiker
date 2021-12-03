#define KL_LOG KL_VM
#include <sys/context.h>
#include <sys/cpu.h>
#include <sys/interrupt.h>
#include <sys/klog.h>
#include <sys/thread.h>
#include <riscv/mcontext.h>
#include <riscv/riscvreg.h>

#define __sp()                                                                 \
  ({                                                                           \
    uint64_t __rv;                                                             \
    __asm __volatile("mv %0, sp" : "=r"(__rv));                                \
    __rv;                                                                      \
  })

__no_profile static inline bool ctx_interrupt(ctx_t *ctx) {
  return _REG(ctx, CAUSE) & SCAUSE_INTR;
}

__no_profile static inline unsigned ctx_code(ctx_t *ctx) {
  return _REG(ctx, CAUSE) & SCAUSE_CODE;
}

__no_profile void trap_handler(ctx_t *ctx) {
  thread_t *td = thread_self();
  assert(td->td_idnest == 0);
  assert(cpu_intr_disabled());

  bool user_mode = user_mode_p(ctx);

  if (!user_mode) {
    /* If there's not enough space on the stack to store another exception
     * frame we consider situation to be critical and panic.
     * Hopefully sizeof(ctx_t) bytes of unallocated stack space will be enough
     * to display error message. */
    register_t sp = __sp();
    if ((sp & (PAGESIZE - 1)) < sizeof(ctx_t))
      panic("Kernel stack overflow caught at %p!", _REG(ctx, PC));
  }

  if (ctx_interrupt(ctx)) {
    intr_root_handler(ctx);
  } else {
    panic("Not implemented!");
  }
}
