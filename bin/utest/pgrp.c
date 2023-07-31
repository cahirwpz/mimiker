#include "utest.h"
#include "util.h"

#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

TEST_ADD(setpgid, 0) {
  pgid_t parent_pgid = getpgid(0);

  pid_t children_pid = xfork();
  if (children_pid == 0) {
    /* Process inherits group of its parent. */
    assert(getpgid(0) == parent_pgid);
    /* Process can make its own group. */
    assert(!setpgid(0, 0));
    /* New group has ID equal to the ID of the moved process. */
    assert(getpgid(0) == getpid());

    exit(0);
  }
  wait_child_finished(children_pid);

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

TEST_ADD(setpgid_leader, 0) {
  signal_setup(SIGUSR1);

  pid_t cpid = xfork();
  if (cpid == 0) {
    /* Become session leader. */
    assert(setsid() == getpid());
    /* Can't change pgrp of session leader. */
    assert(setpgid(0, 0));

    xkill(getppid(), SIGUSR1);
    wait_for_signal(SIGUSR1);
    return 0;
  }

  /* Wait until child becomes session leader. */
  wait_for_signal(SIGUSR1);
  assert(getsid(cpid) == cpid);

  /* Can't change pgrp of session leader. */
  assert(setpgid(cpid, getpgid(0)));

  xkill(cpid, SIGUSR1);

  wait_child_finished(cpid);
  return 0;
}

TEST_ADD(setpgid_child, 0) {
  signal_setup(SIGUSR1); /* child 1 */
  signal_setup(SIGUSR2); /* child 2 */

  pid_t cpid1 = xfork();
  if (cpid1 == 0) {
    /* Become session leader. */
    assert(setsid() == getpid());

    /* Signal readiness to parent. */
    xkill(getppid(), SIGUSR1);

    /* Wait until our parent gives a signal to exit. */
    wait_for_signal(SIGUSR1);
    return 0;
  }

  pid_t cpid2 = xfork();
  if (cpid2 == 0) {
    /* Signal readiness to parent. */
    xkill(getppid(), SIGUSR2);

    /* A child should not be able to change its parent's
     * process group. */
    assert(setpgid(getppid(), 0));

    /* Wait until our parent gives a signal to exit. */
    wait_for_signal(SIGUSR2);
    return 0;
  }

  /* Wait for child 1 to become session leader. */
  wait_for_signal(SIGUSR1);
  assert(getsid(cpid1) == cpid1);

  /* Wait until child 2 is ready. */
  wait_for_signal(SIGUSR2);

  /* Move child 2 into its own process group. */
  assert(!setpgid(cpid2, cpid2));
  assert(getpgid(cpid2) == cpid2);

  /* Move child 2 back to our process group. */
  assert(!setpgid(cpid2, getpgid(0)));
  assert(getpgid(cpid2) == getpgid(0));

  /* Moving child 2 to a process group in a different session should fail. */
  assert(setpgid(cpid2, cpid1));
  assert(getpgid(cpid2) == getpgid(0));

  xkill(cpid1, SIGUSR1);
  xkill(cpid2, SIGUSR2);

  wait_child_finished(cpid1);
  wait_child_finished(cpid2);
  return 0;
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
TEST_ADD(kill, 0) {
  kill_tests_setup();
  pgid_t parent_pid = getpid();

  pid_t pid = xfork();
  if (pid == 0) {
    xkill(parent_pid, SIGUSR1);

    /* Signal is not delivered to all processes in the group. */
    assert(!sig_delivered);
    exit(0);
  }

  wait_child_finished(pid);
  /* Signal is delivered to appropriate process. */
  assert(sig_delivered);

  return 0;
}

/* In this tests there are two processes marked with: a, b.
 * Processes a and b are in the same process group.
 * Process b sends signal to its own process group containing a and b. */
TEST_ADD(killpg_same_group, 0) {
  kill_tests_setup();

  pid_t pid_a = xfork();
  if (pid_a == 0) {
    setpgid(0, 0);
    pid_t pgid_a = getpgid(0);

    pid_t pid_b = xfork();
    if (pid_b == 0) {
      xkillpg(pgid_a, SIGUSR1);

      /* Process b should receive signal from process b. */
      assert(sig_delivered);
      exit(0); // process b
    }

    wait_child_finished(pid_b);
    /* Process a should receive signal from process b. */
    assert(sig_delivered);
    exit(0); // process a
  }

  wait_child_finished(pid_a);
  /* Invalid argument. */
  syscall_fail(killpg(1, SIGUSR1), ESRCH);
  /* Invalid argument (negative number). */
  syscall_fail(killpg(-1, SIGUSR1), ESRCH);

  return 0;
}

/* In this tests there are three processes marked with: a, b, c.
 * Processes a and b are in the same process group.
 * Process c is in different process group than a and b.
 * Process c sends signal to the process group containing a and b. */
TEST_ADD(killpg_other_group, 0) {
  kill_tests_setup();

  pid_t pid_a = xfork();
  if (pid_a == 0) {
    setpgid(0, 0);
    pid_t pgid_a = getpgid(0);

    pid_t pid_b = xfork();
    if (pid_b == 0) {

      pid_t pid_c = xfork();
      if (pid_c == 0) {

        setpgid(0, 0);
        xkillpg(pgid_a, SIGUSR1);

        /* Process c should not receive signal from process c. */
        assert(!sig_delivered);
        exit(0); // process c
      }

      wait_child_finished(pid_c);
      /* Process b should receive signal from process c. */
      assert(sig_delivered);
      exit(0); // process b
    }

    wait_child_finished(pid_b);
    /* Process a should receive signal from process c. */
    assert(sig_delivered);
    exit(0); // process a
  }

  wait_child_finished(pid_a);
  /* It is forbidden to send signal to non-existing group. */
  syscall_fail(killpg(pid_a, SIGUSR1), ESRCH);

  return 0;
}

TEST_ADD(pgrp_orphan, 0) {
  signal_setup(SIGHUP);
  int ppid = getpid();
  pid_t cpid = xfork();
  if (cpid == 0) {
    cpid = getpid();
    assert(setsid() == cpid);
    pid_t gcpid = xfork();

    if (gcpid == 0) {
      gcpid = getpid();
      assert(setpgid(0, 0) == 0);

      raise(SIGSTOP);
      wait_for_signal(SIGHUP);
      xkill(ppid, SIGHUP);
      return 0;
    }

    /* Wait for the grandchild to stop, then orphan its process group. */
    debug("Child: waiting for the grandchild to stop...");
    wait_child_stopped(gcpid);
    /* When we exit, init will become the grandchild's parent.
     * Since init is in a different session, and the grandchild will
     * be the only member of its own process group, the grandchild's
     * process group will be orphaned, which should result in the
     * grandchild receiving SIGHUP and SIGCONT. */
    return 0;
  }

  /* Reap the child. */
  debug("Parent: waiting for the child to exit...");
  wait_child_finished(cpid);

  /* Wait for a signal from the grandchild. */
  debug("Parent: waiting for a signal from the grandchild...");
  wait_for_signal(SIGHUP);

  /* We're exiting without reaping the grandchild.
   * Hopefully init will take care of it. */
  return 0;
}

TEST_ADD(session_basic, 0) {
  signal_setup(SIGUSR1);
  pid_t parent_sid = getsid(getpid());
  assert(parent_sid != -1);
  pid_t cpid = xfork();
  if (cpid == 0) {
    cpid = getpid();
    pid_t ppid = getppid();
    /* Should be in the same session as parent. */
    assert(getsid(0) == parent_sid);
    assert(getsid(ppid) == parent_sid);
    /* Create a session. This should always succeed. */
    assert(setsid() == cpid);
    xkill(ppid, SIGUSR1);
    assert(getsid(0) == cpid);
    assert(getsid(ppid) == parent_sid);
    /* Creating a session when we're already a leader should fail. */
    assert(setsid() == -1);
    assert(getsid(0) == cpid);
    assert(getsid(ppid) == parent_sid);
    /* Hang around for the parent to check our SID. */
    wait_for_signal(SIGUSR1);
    return 0;
  }

  wait_for_signal(SIGUSR1);
  assert(getsid(cpid) == cpid);

  xkill(cpid, SIGUSR1);
  wait_child_finished(cpid);
  return 0;
}

TEST_ADD(session_login_name, 0) {
  const char *name = "foo";
  /* Assume login name is not set. */
  assert(getlogin() == NULL);
  /* Assume tests are run as root, otherwise this should fail. */
  assert(setlogin(name) == 0);
  assert(getlogin() != NULL);
  assert(strcmp(getlogin(), name) == 0);
  /* Name too long: should fail with EINVAL. */
  assert(setlogin("this_is_a_very_long_name") == -1);
  assert(errno == EINVAL);
  /* Failure shouldn't affect the login name. */
  assert(getlogin() != NULL);
  assert(strcmp(getlogin(), name) == 0);
  /* Drop root privileges: setlogin() shoud fail with EPERM. */
  assert(seteuid(1) == 0);
  assert(setlogin(name) == -1);
  assert(errno == EPERM);
  /* getlogin() should succeed with dropped privileges. */
  assert(getlogin() != NULL);
  assert(strcmp(getlogin(), name) == 0);
  /* Restore root privileges. */
  assert(seteuid(0) == 0);
  /* Invalid pointer: setlogin() should fail with EFAULT. */
  assert(setlogin((void *)1) == -1);
  assert(errno == EFAULT);
  /* Failure shouldn't affect the login name. */
  assert(getlogin() != NULL);
  assert(strcmp(getlogin(), name) == 0);
  /* Setting the name to an empty string should "unset" it. */
  assert(setlogin("") == 0);
  assert(getlogin() == NULL);
  return 0;
}
