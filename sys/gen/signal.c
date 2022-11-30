#include <sys/signal.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/ucontext.h>

void sig_trap(ctx_t *ctx, signo_t sig, int code, void *addr, int trapno) {
  proc_t *proc = proc_self();

  ksiginfo_t ksi;
  ksi.ksi_flags = KSI_TRAP;
  ksi.ksi_signo = sig;
  ksi.ksi_code = code;
  ksi.ksi_addr = addr;
  ksi.ksi_trap = trapno;

  WITH_MTX_LOCK (&proc->p_lock)
    sig_kill(proc, &ksi);
}
