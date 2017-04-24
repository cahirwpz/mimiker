#include <exception.h>
#include <thread.h>
#include <sched.h>

void exc_before_leave(exc_frame_t *kframe) {
  thread_t *td = thread_self();

  td->td_kframe = kframe;

  if (td->td_flags & TDF_NEEDSWITCH)
    sched_switch(NULL);

  /* First thing after switching to a thread: Process pending signals. */
  if (td->td_flags & TDF_NEEDSIGCHK) {
    mtx_scoped_lock(&td->td_lock);
    int sig;
    while ((sig = issignal(td)) != 0)
      postsig(sig);
    td->td_flags &= ~TDF_NEEDSIGCHK;
  }
}
