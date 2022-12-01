#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <setjmp.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/fcntl.h>

/* Variables to pass data communicate with signal handler */
volatile void *sigsegv_address = 0;
volatile int sigsegv_code = 0;
jmp_buf return_to;

static void sigsegv_handler(int signo, siginfo_t *info, void *uctx) {
  sigsegv_address = info->si_addr;
  sigsegv_code = info->si_code;
  siglongjmp(return_to, 1);
}

void setup_sigsegv_sigaction(void) {
  sigaction_t sa;
  memset(&sa, 0, sizeof(sigaction_t));
  sa.sa_sigaction = sigsegv_handler;
  sa.sa_flags = SA_SIGINFO;
  int err = sigaction(SIGSEGV, &sa, NULL);
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
