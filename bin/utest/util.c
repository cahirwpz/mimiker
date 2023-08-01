#include "utest.h"
#include "util.h"

#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/wait.h>

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

static void noop_handler(int signo) {
}

void signal_setup(int signo) {
  xsignal(signo, noop_handler);
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, signo);
  xsigprocmask(SIG_BLOCK, &mask, NULL);
}

void wait_for_signal(int signo) {
  sigset_t mask;
  sigfillset(&mask);
  sigdelset(&mask, signo);
  sigsuspend(&mask);
}

void open_pty(int *master_fd, int *slave_fd) {
  *master_fd = posix_openpt(O_NOCTTY | O_RDWR);
  assert(*master_fd >= 0);
  *slave_fd = xopen(ptsname(*master_fd), O_NOCTTY | O_RDWR);
}

/*
 * Layout of mapped pages:
 * 0       1        2        3        4        5       6
 * +----------------+        +--------+        +-----------------+
 * | first | second |        | fourth |        | sixth | seventh |
 * +----------------+        +--------+        +-----------------+
 */
void *prepare_layout(size_t pgsz, int prot) {
  siginfo_t si;
  char *addr = mmap_anon_priv(NULL, 7 * pgsz, prot);

  assert(munmap(addr + 2 * pgsz, pgsz) == 0);
  assert(munmap(addr + 4 * pgsz, pgsz) == 0);

  EXPECT_SIGNAL(SIGSEGV, &si) {
    *(addr + 2 * pgsz) = 1;
  }
  CLEANUP_SIGNAL();
  CHECK_SIGSEGV(&si, addr + 2 * pgsz, SEGV_MAPERR);

  EXPECT_SIGNAL(SIGSEGV, &si) {
    *(addr + 4 * pgsz) = 1;
  }
  CLEANUP_SIGNAL();
  CHECK_SIGSEGV(&si, addr + 4 * pgsz, SEGV_MAPERR);

  return addr;
}
