#include <signal.h>
#include <thread.h>
#include <stdc.h>

int do_kill(tid_t tid, signo_t sig) {
  log("KILLING");
  return 0;
}

int do_sigaction(signo_t sig, const sigaction_t *act, sigaction_t *oldact) {
  log("SIGACTION");
  return 0;
}
