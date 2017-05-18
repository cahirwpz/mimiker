#include <mips/intr.h>
#include <stdc.h>
#include <sync.h>
#include <thread.h>

unsigned _critical_enter() {
  thread_t *td = thread_self();
  if (td->td_csnest++ == 0)
    intr_disable();
  return td->td_csnest;
}

void critical_enter() __alias(_critical_enter);

void critical_leave() {
  thread_t *td = thread_self();
  assert(td->td_csnest > 0);
  if (--td->td_csnest == 0)
    intr_enable();
}
