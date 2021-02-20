#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <sys/wait.h>

#include "utest.h"

/* ======= signal_basic ======= */
static volatile int sigusr1_handled = 0;
static void sigusr1_handler(int signo) {
  printf("sigusr1 handled!\n");
  sigusr1_handled = 1;
}
static void sigint_handler(int signo) {
  printf("sigint handled!\n");
  raise(SIGUSR1); /* Recursive signals! */
}
int test_signal_basic(void) {
  signal(SIGINT, sigint_handler);
  signal(SIGUSR1, sigusr1_handler);
  raise(SIGINT);
  assert(sigusr1_handled);

  /* Restore original behavior. */
  signal(SIGINT, SIG_DFL);
  signal(SIGUSR1, SIG_DFL);

  return 0;
}

/* ======= signal_send ======= */
static void sigusr2_handler(int signo) {
  printf("Child process handles sigusr2.\n");
  raise(SIGABRT); /* Terminate self. */
}
/* Test sending a signal to a different thread. */
int test_signal_send(void) {
  /* The child should inherit signal handler configuration. */
  signal(SIGUSR2, sigusr2_handler);
  int pid = fork();
  if (pid == 0) {
    printf("This is child (mypid = %d)\n", getpid());
    /* Wait for signal. */
    while (1)
      sched_yield();
  }

  printf("This is parent (childpid = %d, mypid = %d)\n", pid, getpid());
  kill(pid, SIGUSR2);
  int status;
  printf("Waiting for child...\n");
  wait(&status);
  assert(WIFSIGNALED(status));
  assert(WTERMSIG(status) == SIGABRT);
  printf("Child was stopped by SIGABRT.\n");
  return 0;
}

/* ======= signal_abort ======= */
/* This test shall be considered success if the process gets terminated with
   SIGABRT */
int test_signal_abort(void) {
  raise(SIGABRT);
  return 0;
}

/* ======= signal_segfault ======= */
/* This test shall be considered success if the process gets terminated with
   SIGABRT */
int test_signal_segfault(void) {
  volatile struct { int x; } *ptr = 0x0;
  ptr->x = 42;
  return 0;
}

/* ======= signal_stop ======= */
static volatile int sigcont_handled = 0;
static void sigcont_handler(int signo) {
  sigcont_handled = 1;
}

static volatile int ppid;
static void signal_parent(int signo) {
  kill(ppid, SIGCONT);
}

int test_signal_stop(void) {
  ppid = getpid();
  signal(SIGUSR1, SIG_IGN);
  signal(SIGCONT, sigcont_handler);
  int pid = fork();
  if (pid == 0) {
    signal(SIGUSR1, signal_parent);
    /* The child keeps sending SIGUSR1 to the parent. */
    while (!sigcont_handled)
      kill(ppid, SIGUSR1);
    return 0;
  }

  int status;
  signal(SIGUSR1, sigusr1_handler);
  /* Wait for the child to start sending signals */
  while (!sigusr1_handled)
    sched_yield();
  kill(pid, SIGSTOP);
  /* Wait for the child to stop. */
  assert(waitpid(pid, &status, WUNTRACED) == pid);
  assert(WIFSTOPPED(status));
  /* Now we shouldn't be getting any signals from the child. */
  sigusr1_handled = 0;
  /* Yield a couple times to make sure that if the child was runnable,
   * it would send us a signal here. */
  for (int i = 0; i < 3; i++)
    sched_yield();
  assert(!sigusr1_handled);
  /* Stopped processes shouldn't handle incoming signals until they're
   * continued (with SIGKILL and SIGCONT being the only exceptions).
   * Send SIGUSR1 to the stopped child. If the handler runs, it will
   * send us SIGCONT. */
  kill(pid, SIGUSR1);
  for (int i = 0; i < 3; i++)
    sched_yield();
  assert(!sigcont_handled);
  /* Now continue the child process. */
  kill(pid, SIGCONT);
  /* The child's SIGUSR1 handler should now run, and so our SIGCONT handler
   * should run too. */
  while (!sigcont_handled)
    sched_yield();
  /* The child process should exit normally. */
  printf("Waiting for child...\n");
  wait(&status);
  assert(WIFEXITED(status));
  assert(WEXITSTATUS(status) == 0);
  return 0;
}

/* ======= signal_cont_masked ======= */
int test_signal_cont_masked(void) {
  ppid = getpid();
  signal(SIGCONT, sigcont_handler);
  int pid = fork();
  if (pid == 0) {
    /* Block SIGCONT. */
    sigset_t mask, old;
    __sigemptyset(&mask);
    sigaddset(&mask, SIGCONT);
    assert(sigprocmask(SIG_BLOCK, &mask, &old) == 0);
    /* Even though SIGCONT is blocked, it should wake us up, but it
     * should remain pending until we unblock it. */
    raise(SIGSTOP);
    assert(!sigcont_handled);
    /* Unblock SIGCONT: the handler should run immediately after. */
    assert(sigprocmask(SIG_SETMASK, &old, NULL) == 0);
    assert(sigcont_handled);
    return 0;
  }

  int status;
  /* Wait for the child to stop. */
  assert(waitpid(pid, &status, WUNTRACED) == pid);
  assert(WIFSTOPPED(status));

  kill(pid, SIGCONT);
  printf("Waiting for child...\n");
  wait(&status);
  assert(WIFEXITED(status));
  assert(WEXITSTATUS(status) == 0);
  return 0;
}

/* ======= signal_mask ======= */
int test_signal_mask(void) {
  ppid = getpid();
  signal(SIGUSR1, signal_parent);
  signal(SIGCONT, sigcont_handler);

  int pid = fork();
  if (pid == 0) {
    while (!sigcont_handled)
      sched_yield();
    return 0;
  }

  /* Check that the signal bounces properly. */
  kill(pid, SIGUSR1);
  while (!sigcont_handled)
    sched_yield();

  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGCONT);

  /* Mask the signal and make the child send it to us.
   * The delivery of the signal should be delayed until we unblock it. */
  assert(sigprocmask(SIG_BLOCK, &mask, NULL) == 0);
  sigcont_handled = 0;
  kill(pid, SIGUSR1);

  /* Wait until we get a signal from the child. */
  sigset_t set;
  do {
    sched_yield();
    sigpending(&set);
  } while (!__sigismember(&set, SIGCONT));

  assert(!sigcont_handled);

  /* Unblocking a pending signal should make us handle it immediately. */
  assert(sigprocmask(SIG_UNBLOCK, &mask, NULL) == 0);
  assert(sigcont_handled);

  kill(pid, SIGCONT);
  int status;
  printf("Waiting for child...\n");
  wait(&status);
  assert(WIFEXITED(status));
  assert(WEXITSTATUS(status) == 0);
  return 0;
}

/* ======= signal_mask_nonmaskable ======= */
int test_signal_mask_nonmaskable(void) {
  sigset_t set, old;
  __sigemptyset(&set);
  __sigaddset(&set, SIGSTOP);
  __sigaddset(&set, SIGKILL);
  __sigaddset(&set, SIGUSR1);
  /* The call should succeed, but SIGKILL and SIGSTOP shouldn't be blocked. */
  assert(sigprocmask(SIG_BLOCK, &set, &old) == 0);
  assert(sigprocmask(SIG_BLOCK, NULL, &set) == 0);
  __sigaddset(&old, SIGUSR1);
  assert(__sigsetequal(&set, &old));
  return 0;
}

/* ======= signal_sigsuspend ======= */
int test_signal_sigsuspend(void) {
  pid_t ppid = getpid();
  signal(SIGCONT, sigcont_handler);
  signal(SIGUSR1, sigusr1_handler);
  sigset_t set, old;
  __sigemptyset(&set);
  __sigaddset(&set, SIGCONT);
  __sigaddset(&set, SIGUSR1);
  assert(sigprocmask(SIG_BLOCK, &set, &old) == 0);
  __sigaddset(&old, SIGCONT);
  pid_t cpid = fork();
  if (cpid == 0) {
    for (int i = 0; i < 10; i++) {
      kill(ppid, SIGCONT);
      sched_yield();
    }
    kill(ppid, SIGUSR1);
    return 0;
  }
  /* Go to sleep with SIGCONT blocked and SIGUSR1 unblocked. */
  printf("Calling sigsuspend()...\n");
  sigsuspend(&old);
  /* SIGUSR1 should have woken us up, but SIGCONT should still be pending. */
  assert(sigusr1_handled);
  assert(!sigcont_handled);
  __sigemptyset(&set);
  __sigaddset(&set, SIGCONT);
  assert(sigprocmask(SIG_UNBLOCK, &set, NULL) == 0);
  assert(sigcont_handled);

  int status;
  printf("Waiting for child...\n");
  wait(&status);
  assert(WIFEXITED(status));
  assert(WEXITSTATUS(status) == 0);
  return 0;
}

/* ======= signal_sigsuspend_stop ======= */
int test_signal_sigsuspend_stop(void) {
  pid_t ppid = getpid();
  signal(SIGUSR1, sigusr1_handler);
  sigset_t set, old;
  __sigemptyset(&set);
  __sigaddset(&set, SIGUSR1);
  assert(sigprocmask(SIG_BLOCK, &set, &old) == 0);
  pid_t cpid = fork();
  if (cpid == 0) {
    sigsuspend(&old);
    assert(sigusr1_handled);
    kill(ppid, SIGUSR1);
    return 0;
  }
  /* Wait for the child to call sigsuspend().
   * Right now there's no 100% effective way to do this. */
  for (int i = 0; i < 3; i++)
    sched_yield();

  /* Stop the child. */
  kill(cpid, SIGSTOP);
  int status;
  assert(waitpid(cpid, &status, WUNTRACED) == cpid);
  assert(WIFSTOPPED(status));

  /* Continue the child. This should not interrupt the child's sigsuspend(). */
  kill(cpid, SIGCONT);
  /* Give the child a chance to run if it has been resumed
   * (which it shouldn't). */
  for (int i = 0; i < 3; i++)
    sched_yield();

  assert(!sigusr1_handled);

  /* Stop the child again. */
  kill(cpid, SIGSTOP);
  assert(waitpid(cpid, &status, WUNTRACED) == cpid);
  assert(WIFSTOPPED(status));

  /* Send SIGUSR1 to the child. Since it's stopped, it should not interrupt
   * the sigsuspend() yet. */
  kill(cpid, SIGUSR1);
  /* Give the child a chance to run if it has been resumed
   * (which it shouldn't). */
  for (int i = 0; i < 3; i++)
    sched_yield();

  assert(!sigusr1_handled);

  /* Continue the child. Now the SIGUSR1 we sent earlier should interrupt
   * the sigsuspend() call. */
  kill(cpid, SIGCONT);

  /* Wait for the child to send us SIGUSR1. */
  sigsuspend(&old);

  /* Reap the child. */
  wait(&status);
  assert(WIFEXITED(status));
  assert(WEXITSTATUS(status) == 0);
  return 0;
}

/* ======= signal_handler_mask ======= */
static int handler_success;
static int handler_ran;
static pid_t cpid;

static void yield_handler(int signo) {
  /* Give the child process the signal to send us SIGUSR1 */
  kill(cpid, SIGUSR1);
  while (!sigcont_handled)
    sched_yield();
  handler_ran = 1;
  if (!sigusr1_handled)
    handler_success = 1;
}

int test_signal_handler_mask(void) {
  pid_t ppid = getpid();
  struct sigaction sa = {.sa_handler = yield_handler, .sa_flags = 0};
  /* Block SIGUSR1 when executing handler for SIGUSR2. */
  __sigemptyset(&sa.sa_mask);
  __sigaddset(&sa.sa_mask, SIGUSR1);
  assert(sigaction(SIGUSR2, &sa, NULL) == 0);
  signal(SIGUSR1, sigusr1_handler);
  signal(SIGCONT, sigcont_handler);

  pid_t cpid = fork();
  if (cpid == 0) {
    kill(ppid, SIGUSR2);
    /* Wait for the parent to enter the signal handler. */
    while (!sigusr1_handled)
      sched_yield();
    /* Now SIGUSR1 should be blocked in the parent. */
    for (int i = 0; i < 3; i++)
      kill(ppid, SIGUSR1);
    /* Sending SIGCONT should allow yield_handler() to run to completion. */
    kill(ppid, SIGCONT);
    return 0;
  }

  while (!handler_ran)
    sched_yield();

  assert(handler_success);
  assert(sigusr1_handled);

  int status;
  printf("Waiting for child...\n");
  wait(&status);
  assert(WIFEXITED(status));
  assert(WEXITSTATUS(status) == 0);
  return 0;
}
