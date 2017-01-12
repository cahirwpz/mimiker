#include <mips/intr.h>
#include <stdc.h>
#include <sync.h>
#include <thread.h>

void critical_enter() {
  thread_t *td = thread_self();
  if (td->td_csnest++ == 0)
    intr_disable();
}

void critical_leave() {
  thread_t *td = thread_self();
  assert(td->td_csnest > 0);
  if (--td->td_csnest == 0)
    intr_enable();
}
