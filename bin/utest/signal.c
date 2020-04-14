#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
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

static volatile int parent_pid;

static void bounce_sigusr1(int signo) {
  kill(parent_pid, SIGUSR1);
}

static volatile int sigusr2_handled;

static void sigusr2_handler2(int signo) {
  sigusr2_handled = 1;
}

int test_signal_mask() {
  sigusr1_handled = 0;
  sigusr2_handled = 0;
  parent_pid = getpid();

  signal(SIGUSR1, bounce_sigusr1);
  signal(SIGUSR2, sigusr2_handler2);

  int pid = fork();
  if (pid == 0) {
    while (!sigusr2_handled) ;
    return 0;
  };

  signal(SIGUSR1, sigusr1_handler);

  /* Check that the signal bounces properly. */
  kill(pid, SIGUSR1);
  while (!sigusr1_handled);

  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR1);

  /* Mask the signal and make the child send it to us.
   * The delivery of the signal should be delayed until we unblock it. */
  assert(sigprocmask(SIG_BLOCK, &mask, NULL) == 0);
  sigusr1_handled = 0;

  kill(pid, SIGUSR1);

  //sched_yield();
  for (volatile int i = 0; i < 10000000; i++);
  assert(!sigusr1_handled);

  /* Unblocking a pending signal should make us handle it immediately. */
  assert(sigprocmask(SIG_UNBLOCK, &mask, NULL) == 0);
  assert(sigusr1_handled);

  kill(pid, SIGUSR2);
  int status;
  wait(&status);
  assert(WIFEXITED(status));
  assert(WEXITSTATUS(status) == 0);

  return 0;
}
