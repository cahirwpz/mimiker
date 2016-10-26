#include <mips/mips.h>
#include <thread.h>
#include <pcpu.h>

thread_t *thread_self() {
  return PCPU_GET(curthread);
}
