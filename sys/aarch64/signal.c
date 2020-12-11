#define KL_LOG KL_SIGNAL
#include <sys/mimiker.h>
#include <sys/signal.h>
#include <sys/thread.h>
#include <sys/klog.h>
#include <aarch64/context.h>
#include <sys/errno.h>
#include <sys/proc.h>

int sig_send(signo_t sig, sigset_t *mask, sigaction_t *sa, ksiginfo_t *ksi) {
  panic("Not implemented!");
}

int do_sigreturn(ucontext_t *ucp) {
  panic("Not implemented!");
}

void sig_trap(ctx_t *ctx, signo_t sig) {
  panic("Not implemented!");
}
