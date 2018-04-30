#include <exception.h>
#include <interrupt.h>
#include <thread.h>
#include <sched.h>
#include <signal.h>

void exc_before_leave(exc_frame_t *kframe) {
  thread_t *td = thread_self();

  /* If thread requested not to be preempted, then do not switch out! */
  if (td->td_pdnest > 0)
    return;

  intr_enable();

  if (td->td_flags & TDF_NEEDSWITCH) {
    WITH_SPINLOCK(td->td_spin) {
      td->td_state = TDS_READY;
      sched_switch();
    }
  }

  /* First thing after switching to a thread: Process pending signals. */
  if (td->td_flags & TDF_NEEDSIGCHK) {
    SCOPED_MTX_LOCK(&td->td_lock);
    int sig;
    while ((sig = sig_check(td)) != 0)
      sig_deliver(sig);
    td->td_flags &= ~TDF_NEEDSIGCHK;
  }

  intr_disable();
}
