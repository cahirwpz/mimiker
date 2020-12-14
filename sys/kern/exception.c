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
    bool restart = false;
    if (error == ERESTARTSYS) {
      /* Restart iff no signal was caught or caught signal has SA_RESTART set */
      restart = !sig || (p->p_sigactions[sig].sa_flags & SA_RESTART);
    } else {
      /* Restart iff no signal was caught. */
      restart = !sig;
    }

    if (restart) {
      mcontext_restart_syscall(ctx);
      return;
    }
    /* ERESTART* are internal to the kernel. Change error code to EINTR. */
    error = EINTR;
  }

  mcontext_set_retval(ctx, result->retval, error);
}

void on_user_exc_leave(mcontext_t *ctx, syscall_result_t *result) {
  thread_t *td = thread_self();
  proc_t *p = td->td_proc;
  int sig = 0;
  ksiginfo_t ksi;

  /* XXX we need to know if there's a signal to be delivered in order to call
   * set_syscall_retval(), but we also need to call set_syscall_retval() before
   * sig_post(), as set_syscall_retval() assumes the context has not been
   * modified, and sig_post() modifies it. This is why the logic here looks
   * a bit weird. */
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
