#define KL_LOG KL_SYSCALL
#include <sys/klog.h>
#include <sys/errno.h>
#include <sys/thread.h>
#include <sys/proc.h>
#include <sys/sysent.h>
#include <machine/syscall.h>

void syscall_handler(int code, ctx_t *ctx, syscall_result_t *result) {
  register_t args[SYS_MAXSYSARGS];
  const size_t nregs = min(SYS_MAXSYSARGS, FUNC_MAXREGARGS);
  int error = 0;

  /*
   * Copy the arguments passed via register from the
   * trapctx to our argument array
   */
  memcpy(args, sc_md_args(ctx), nregs * sizeof(register_t));

  if (code > SYS_MAXSYSCALL) {
    args[0] = code;
    code = 0;
  }

  sysent_t *se = &sysent[code];

#if SYS_MAXSYSARGS > FUNC_MAXREGARGS
  size_t nargs = se->nargs;

  if (nargs > nregs) {
    error = copyin(sc_md_stack_args(ctx, nregs), &args[nregs],
                   (nargs - nregs) * sizeof(register_t));
  }
#endif

  /* Call the handler. */
  thread_t *td = thread_self();
  register_t retval = 0;

  assert(td->td_proc != NULL);

  if (!error)
    error = se->call(td->td_proc, (void *)args, &retval);

  if (error && error < EJUSTRETURN)
    klog("%s(...) = %d", se->name, error);

  result->retval = error ? -1 : retval;
  result->error = error;
}

void syscall_set_retval(mcontext_t *ctx, syscall_result_t *result, int sig) {
  int error = result->error;
  proc_t *p = proc_self();

  switch (error) {
    case EJUSTRETURN:
      return;

    case ERESTARTSYS:
    case ERESTARTNOHAND: {
      /* Restart iff no signal was caught... */
      if (sig == 0)
        break;

      /* ... or caught signal has SA_RESTART set. */
      if (error == ERESTARTSYS && (p->p_sigactions[sig].sa_flags & SA_RESTART))
        break;

      /* ERESTART* are internal to the kernel. Change error code to EINTR. */
      error = EINTR;

      __fallthrough;
    }

    default:
      mcontext_set_retval(ctx, result->retval, error);
      return;
  }

  mcontext_restart_syscall(ctx);
}
