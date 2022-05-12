#define KL_LOG KL_VM
#include <sys/klog.h>
#include <riscv/mcontext.h>

__no_profile void trap_handler(ctx_t *ctx) {
  panic("Not implemented!");
}
