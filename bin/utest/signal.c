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
int test_signal_basic() {
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
int test_signal_send() {
  /* The child should inherit signal handler configuration. */
  signal(SIGUSR2, sigusr2_handler);
  int pid = fork();
  if (pid == 0) {
    printf("This is child (mypid = %d)\n", getpid());
    /* Wait for signal. */
    while (1)
      ;
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
int test_signal_abort() {
  raise(SIGABRT);
  return 0;
}

/* ======= signal_segfault ======= */
/* This test shall be considered success if the process gets terminated with
   SIGABRT */
int test_signal_segfault() {
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

int test_signal_stop() {
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

  signal(SIGUSR1, sigusr1_handler);
  /* Wait for the child to start sending signals */
  while (!sigusr1_handled)
    sched_yield();
  kill(pid, SIGSTOP);
  /* Yielding should make sure that the child processes the signal.
   * TODO: once it's implemented, use waitpid to wait until the child stops. */
  for (int i = 0; i < 3; i++)
    sched_yield();
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
   * send us a signal. */
  kill(pid, SIGUSR1);
  sched_yield();
  assert(!sigusr1_handled);
  /* Now continue the child process -- it should exit normally. */
  kill(pid, SIGCONT);
  /* The child's SIGUSR1 handler should now run, and so our SIGCONT handler
   * should run too. */
  sched_yield();
  assert(sigcont_handled);
  int status;
  printf("Waiting for child...\n");
  wait(&status);
  assert(WIFEXITED(status));
  assert(WEXITSTATUS(status) == 0);
  return 0;
}
