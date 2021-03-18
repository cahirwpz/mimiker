#include <signal.h>

sigset_t __sigintr;

int siginterrupt(int sig, int flag) {
  int ret;
  struct sigaction act;

  if ((ret = sigaction(sig, NULL, &act)))
    return ret;
  if (flag) {
    act.sa_flags &= ~SA_RESTART;
    __sigaddset(&__sigintr, sig);
  } else {
    act.sa_flags |= SA_RESTART;
    __sigdelset(&__sigintr, sig);
  }
  return sigaction(sig, &act, NULL);
}
