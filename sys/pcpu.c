#include <thread.h>
#include <pcpu.h>

pcpu_t _pcpu_data[1] = {{}};

static thread_t dummy[1] = {
  (thread_t){.td_name = "dummy thread", .td_tid = 0, .td_csnest = 1}};

void pcpu_init(void) {
  PCPU_SET(curthread, dummy);
}
