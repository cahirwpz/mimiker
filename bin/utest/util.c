#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <setjmp.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/fcntl.h>

jmp_buf _expect_signal_ctx;

static int _handled_signo;
static siginfo_t *_save_info;
static sigaction_t _old_sa;

static void signal_handler(int signo, siginfo_t *info, void *uctx) {
  memcpy(_save_info, info, sizeof(siginfo_t));
  siglongjmp(_expect_signal_ctx, 1);
}

void _expect_signal_setup(int signo, siginfo_t *siginfop) {
  sigaction_t sa;
  _handled_signo = signo;
  memset(&sa, 0, sizeof(sigaction_t));
  memset(siginfop, 0, sizeof(siginfo_t));
  sa.sa_sigaction = signal_handler;
  sa.sa_flags = SA_SIGINFO;
  int err = sigaction(_handled_signo, &sa, &_old_sa);
  assert(err == 0);
  _save_info = siginfop;
}

void _expect_signal_cleanup(void) {
  int err = sigaction(_handled_signo, &_old_sa, NULL);
  assert(err == 0);
}

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

void wait_for_signal(int signo) {
  sigset_t mask;
  __sigfillset(&mask);
  sigdelset(&mask, signo);
  sigsuspend(&mask);
}

void open_pty(int *master_fd, int *slave_fd) {
  *master_fd = posix_openpt(O_NOCTTY | O_RDWR);
  assert(*master_fd >= 0);
  *slave_fd = open(ptsname(*master_fd), O_NOCTTY | O_RDWR);
  assert(*slave_fd >= 0);
}
