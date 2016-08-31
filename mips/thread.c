#include <mips/mips.h>
#include <thread.h>

thread_t *thread_self() {
  return (thread_t *)mips32_get_c0(C0_USERLOCAL);
}
