#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sched.h>
#include <stdio.h>

#include "utest.h"

int test_setpgid(void) {
  /* Process can create and enter new process group. */
  pid_t parent_pid = getpid();
  assert(!setpgid(parent_pid, 0));

  /* setpgid(pid, 0) translates to setpgid(pid, pid). */
  pgid_t parent_pgid = getpgid(0);
  assert(parent_pgid == parent_pid);

  pid_t children_pid = fork();
  if (children_pid == 0) {
    /* Process inherits group of its parent. */
    assert(getpgid(0) == parent_pgid);
    /* Process can make its own group. */
    assert(!setpgid(0, 0));
    /* New group has ID equal to the ID of the moved process. */
    assert(getpgid(0) == getpid());

    exit(0);
  }
  wait(NULL);

  /* It is forbidden to move the process to non-existing group. */
  assert(setpgid(0, children_pid));
  /* It is forbidden to change the group of non-existing process. */
  assert(setpgid(children_pid, children_pid));

  return 0;
}

static sig_atomic_t sig_delivered = 0;

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

/* In this test child process sends signal to its parent. */
int test_kill(void) {
  kill_tests_setup();
  pgid_t parent_pid = getpid();

  pid_t pid = fork();
  if (pid == 0) {
    kill(parent_pid, SIGUSR1);

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

static volatile int sighup_handled;
static void sighup_handler(int signo) {
  sighup_handled = 1;
}

int test_pgrp_orphan() {
  signal(SIGHUP, sighup_handler);
  int ppid = getpid();
  pid_t cpid = fork();
  int status;
  if (cpid == 0) {
    cpid = getpid();
    assert(setsid() == cpid);
    pid_t gcpid = fork();

    if (gcpid == 0) {
      gcpid = getpid();
      assert(setpgid(0, 0) == 0);

      raise(SIGSTOP);
      while (!sighup_handled)
        sched_yield();
      kill(ppid, SIGHUP);
      return 0;
    }

    /* Wait for the grandchild to stop, then orphan its process group. */
    printf("Child: waiting for the grandchild to stop...\n");
    assert(waitpid(gcpid, &status, WUNTRACED) == gcpid);
    assert(WIFSTOPPED(status));
    /* When we exit, init will become the grandchild's parent.
     * Since init is in a different session, and the grandchild will
     * be the only member of its own process group, the grandchild's
     * process group will be orphaned, which should result in the
     * grandchild receiving SIGHUP and SIGCONT. */
    return 0;
  }

  /* Reap the child. */
  printf("Parent: waiting for the child to exit...\n");
  assert(waitpid(cpid, &status, 0) == cpid);
  assert(WIFEXITED(status));
  assert(WEXITSTATUS(status) == 0);

  /* Wait for a signal from the grandchild. */
  printf("Parent: waiting for a signal from the grandchild...\n");
  while (!sighup_handled)
    sched_yield();

  /* We're exiting without reaping the grandchild.
   * Hopefully init will take care of it. */
  return 0;
}

static volatile pid_t parent_sid;

int test_session_basic() {
  signal(SIGUSR1, sa_handler);
  parent_sid = getsid(getpid());
  assert(parent_sid != -1);
  pid_t cpid = fork();
  if (cpid == 0) {
    cpid = getpid();
    pid_t ppid = getppid();
    /* Should be in the same session as parent. */
    assert(getsid(0) == parent_sid);
    assert(getsid(ppid) == parent_sid);
    /* Create a session. This should always succeed. */
    assert(setsid() == cpid);
    assert(getsid(0) == cpid);
    assert(getsid(ppid) == parent_sid);
    /* Creating a session when we're already a leader should fail. */
    assert(setsid() == -1);
    assert(getsid(0) == cpid);
    assert(getsid(ppid) == parent_sid);
    /* Hang around for the parent to check our SID. */
    while (!sig_delivered)
      sched_yield();
    return 0;
  }

  pid_t child_sid;
  while ((child_sid = getsid(cpid)) != cpid) {
    assert(child_sid == parent_sid);
    sched_yield();
  }

  kill(cpid, SIGUSR1);
  int status;
  wait(&status);
  assert(WIFEXITED(status));
  assert(WEXITSTATUS(status) == 0);
  return 0;
}
