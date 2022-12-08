#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <setjmp.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/fcntl.h>

jmp_buf signal_return;
static int _handled_signo;
static siginfo_t *_save_info;
static sigaction_t _old_sa;

static void signal_handler(int signo, siginfo_t *info, void *uctx) {
  memcpy(_save_info, info, sizeof(siginfo_t));
  siglongjmp(signal_return, 1);
}

void setup_handler(int signo, siginfo_t *siginfo_ptr) {
  sigaction_t sa;
  _handled_signo = signo;
  memset(&sa, 0, sizeof(sigaction_t));
  memset(siginfo_ptr, 0, sizeof(siginfo_t));
  sa.sa_sigaction = signal_handler;
  sa.sa_flags = SA_SIGINFO;
  int err = sigaction(_handled_signo, &sa, &_old_sa);
  assert(err == 0);
  _save_info = siginfo_ptr;
}

void check_signal(int signo, void *addr, int code) {
  assert(_save_info->si_signo == signo);
  assert(_save_info->si_addr == addr);
  assert(_save_info->si_code == code);
}

void restore_handler(void) {
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
