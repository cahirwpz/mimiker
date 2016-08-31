#include <thread.h>
#include <sync.h>
#include <mips/mips.h>
#include <stdc.h>

void cs_enter() {
  thread_t *td = thread_self();
  if (td->td_csnest++ == 0)
    intr_disable();
}

void cs_leave() {
  thread_t *td = thread_self();
  assert(td->td_csnest > 0);
  if (--td->td_csnest == 0)
    intr_enable();
}
