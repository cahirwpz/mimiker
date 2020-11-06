#include <sys/exception.h>
#include <sys/thread.h>
#include <sys/sched.h>
#include <sys/proc.h>
#include <sys/signal.h>

void on_exc_leave(void) {
  /* If thread requested not to be preempted, then do not switch out! */
  if (preempt_disabled())
    return;

  thread_t *td = thread_self();
  if (td->td_flags & TDF_NEEDSWITCH) {
    spin_lock(td->td_lock);
    td->td_state = TDS_READY;
    sched_switch();
  }
}

void on_user_exc_leave(void) {
  thread_t *td = thread_self();

  proc_t *p = td->td_proc;
  /* Process pending signals. */
  if (td->td_flags & TDF_NEEDSIGCHK) {
    WITH_PROC_LOCK(p) {
      /* Calling sig_post() multiple times before returning to userspace
       * will not make us lose signals, see comment on sig_post() in signal.h */
      ksiginfo_t ksi;
      while (sig_check(td, &ksi))
        sig_post(&ksi);
    }
  }
}
