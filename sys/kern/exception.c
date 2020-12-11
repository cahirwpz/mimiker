#include <sys/context.h>
#include <sys/errno.h>
#include <sys/sysent.h>
#include <sys/siginfo.h>
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

static void set_syscall_retval(mcontext_t *ctx, syscall_result_t *result,
                               int sig) {
  int error = result->error;
  proc_t *p = proc_self();

  if (error == EJUSTRETURN)
    return;

  if (error == ERESTARTSYS || error == ERESTARTNOHAND) {
    if (!sig || (error == ERESTARTSYS &&
                 (p->p_sigactions[sig].sa_flags & SA_RESTART))) {
      mcontext_restart_syscall(ctx);
      return;
    }
    error = EINTR;
  }

  mcontext_set_retval(ctx, result->retval, error);
}

void on_user_exc_leave(mcontext_t *ctx, syscall_result_t *result) {
  thread_t *td = thread_self();
  proc_t *p = td->td_proc;
  int sig = 0;
  ksiginfo_t ksi;

  if (td->td_flags & TDF_NEEDSIGCHK) {
    WITH_PROC_LOCK(p) {
      sig = sig_check(td, &ksi);
    }
  }

  /* Set return value from syscall, restarting it if needed. */
  if (result)
    set_syscall_retval(ctx, result, sig);

  /* Process pending signals. */
  while (sig) {
    WITH_PROC_LOCK(p) {
      /* Calling sig_post() multiple times before returning to userspace
       * will not make us lose signals, see comment on sig_post() in signal.h
       */
      sig_post(&ksi);
      sig = sig_check(td, &ksi);
    }
  }
}
