#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <setjmp.h>
#include <stdnoreturn.h>

static sigjmp_buf jump_buffer;

noreturn static void sigint_handler(int signo) {
  siglongjmp(jump_buffer, 42);
  assert(0); /* Shouldn't reach here. */
}

int test_sigaction_with_setjmp(void) {
  struct sigaction sa;

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = sigint_handler;
  assert(sigaction(SIGINT, &sa, NULL) == 0);

  if (sigsetjmp(jump_buffer, 1) != 42) {
    raise(SIGINT);
    assert(0); /* Shouldn't reach here. */
  }

  return 0;
}

static volatile int sigusr1_handled;

static void sigusr1_handler(int signo, siginfo_t *si, void *uctx) {
  assert(si->si_signo == signo);
  assert(si->si_code == SI_NOINFO);
  assert(uctx != NULL);
  sigusr1_handled = 1;
}

int test_sigaction_handler_returns(void) {
  struct sigaction sa;

  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = sigusr1_handler;
  sa.sa_flags = SA_SIGINFO;
  assert(sigaction(SIGUSR1, &sa, NULL) == 0);

  assert(sigusr1_handled == 0);
  raise(SIGUSR1);
  assert(sigusr1_handled == 1);

  return 0;
}

#define NCHLD 7

static volatile int nreaped;

static void sigchld_handler(int signo, siginfo_t *si, void *uctx) {
  assert(si->si_code == CLD_EXITED);

  printf("child=%d (uid=%d) sent signal info\n", si->si_pid, si->si_uid);

  while (waitpid(-1, NULL, WNOHANG) > 0)
    nreaped++;
}

int test_sigaction_siginfo_from_children(void) {
  struct sigaction sa;

  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = sigchld_handler;
  sa.sa_flags = SA_SIGINFO;
  assert(sigaction(SIGCHLD, &sa, NULL) == 0);

  sigset_t set, old;
  __sigemptyset(&set);
  __sigaddset(&set, SIGCHLD);
  assert(sigprocmask(SIG_BLOCK, &set, &old) == 0);

  for (int i = 0; i < NCHLD; i++) {
    pid_t pid = fork();
    assert(pid >= 0);
    if (!pid) {
      sleep(i & 0x1);
      _exit(0);
    }
  }

  assert(nreaped == 0);

  do {
    sigsuspend(&old);
  } while (nreaped != NCHLD);

  return 0;
}
