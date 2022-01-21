#define KL_LOG KL_SIGNAL
#include <sys/klog.h>
#include <sys/signal.h>

int sig_send(signo_t sig, sigset_t *mask, sigaction_t *sa, ksiginfo_t *ksi) {
  panic("Not implemented!");
}

void sig_trap(ctx_t *ctx, signo_t sig) {
  panic("Not implemented!");
}
