#include <assert.h>
#include <signal.h>
#include <sys/wait.h>

void wait_for_child_exit(int pid, int exit_code) {
  int status;
  assert(waitpid(pid, &status, 0) == pid);
  assert(WIFEXITED(status));
  assert(WEXITSTATUS(status) == exit_code);
}

static void noop_handler(int signo) {
}

void signal_setup(int signo) {
  signal(signo, noop_handler);
  sigset_t mask;
  __sigemptyset(&mask);
  sigaddset(&mask, signo);
  assert(sigprocmask(SIG_BLOCK, &mask, NULL) == 0);
}

void wait_for_signal(int signo, sigset_t *oldmask) {
  sigset_t mask;
  __sigfillset(&mask);
  sigdelset(&mask, signo);
  sigsuspend(&mask);
}
