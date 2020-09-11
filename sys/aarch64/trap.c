#include <sys/context.h>
#include <sys/mimiker.h>

void cpu_user_trap_handler(user_ctx_t *uctx) {
  panic("cpu_user_trap_handler");
}

void cpu_kern_trap_handler(ctx_t *ctx) {
  panic("cpu_kern_trap_handler");
}
