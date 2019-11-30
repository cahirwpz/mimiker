#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "utest.h"

int test_setpgid(void) {
  /* Init process can create and enter new process group. */
  assert(!setpgid(1, 0));

  pid_t pid = fork();
  if (pid == 0) {
    /* Process inherits group of its parent. */
    assert(getpgid(0) == 1);
    /* Process can make its own group. */
    assert(!setpgid(0, 0));
    /* New group has ID equal to the ID of the moved process. */
    assert(getpgid(0) == getpid());

    exit(0);
  }
  wait(NULL);

  /* It is forbidden to move the process to non-existing group. */
  assert(setpgid(0, pid));
  /* It is forbidden to change the group of non-existing process. */
  assert(setpgid(pid, pid));

  return 0;
}

static int sig_delivered = 0;

static void sa_handler(int signo) {
  assert(signo == SIGUSR1);
  sig_delivered = 1;
}

static void kill_tests_setup(void) {
  sig_delivered = 0;
  setpgid(0, 1);

  sigaction_t newact;
  sigaction_t oldact;

  newact.sa_handler = sa_handler;
  sigaction(SIGUSR1, &newact, &oldact);
}

/* In this test parent process sends signal to its child. */
int test_kill(void) {
  kill_tests_setup();

  pid_t pid = fork();
  if (pid == 0) {
    kill(1, SIGUSR1);
    /* Signal is not delivered to all processes in the group. */
    assert(!sig_delivered);
    exit(0);
  }

  wait(NULL);
  /* Signal is delivered to appropriate process. */
  assert(sig_delivered);

  return 0;
}

/* In this tests there are two processes marked with: a, b.
 * Processes a and b are in the same process group.
 * Process b sends signal to its own process group containing a and b. */
int test_killpg_same_group(void) {
  kill_tests_setup();

  pid_t pid_a = fork();
  if (pid_a == 0) {
    setpgid(0, 0);
    pid_t pgid_a = getpgid(0);

    pid_t pid_b = fork();
    if (pid_b == 0) {
      assert(!killpg(pgid_a, SIGUSR1));

      /* Process b should receive signal from process b. */
      assert(sig_delivered);
      exit(0); // process b
    }

    wait(NULL);
    /* Process a should receive signal from process b. */
    assert(sig_delivered);
    exit(0); // process a
  }

  wait(NULL);
  /* Invalid argument. */
  assert(killpg(1, SIGUSR1));
  /* Invalid argument (negative number). */
  assert(killpg(-1, SIGUSR1));

  return 0;
}

/* In this tests there are three processes marked with: a, b, c.
 * Processes a and b are in the same process group.
 * Process c is in different process group than a and b.
 * Process c sends signal to the process group containing a and b. */
int test_killpg_other_group(void) {
  kill_tests_setup();

  pid_t pid_a = fork();
  if (pid_a == 0) {
    setpgid(0, 0);
    pid_t pgid_a = getpgid(0);

    pid_t pid_b = fork();
    if (pid_b == 0) {

      pid_t pid_c = fork();
      if (pid_c == 0) {

        setpgid(0, 0);
        assert(!killpg(pgid_a, SIGUSR1));

        /* Process c should not receive signal from process c. */
        assert(!sig_delivered);
        exit(0); // process c
      }

      wait(NULL);
      /* Process b should receive signal from process c. */
      assert(sig_delivered);
      exit(0); // process b
    }

    wait(NULL);
    /* Process a should receive signal from process c. */
    assert(sig_delivered);
    exit(0); // process a
  }

  wait(NULL);
  /* It is forbidden to send signal to non-existing group. */
  assert(killpg(pid_a, SIGUSR1));

  return 0;
}
