#include "utest.h"
#include "util.h"

#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

/* ======= signal_basic ======= */
static volatile int sigusr1_handled = 0;

static void sigusr1_handler(int signo) {
  debug("sigusr1 handled!");
  sigusr1_handled = 1;
}

static void sigint_handler(int signo) {
  debug("sigint handled!");
  raise(SIGUSR1); /* Recursive signals! */
}

TEST_ADD(signal_basic, 0) {
  xsignal(SIGINT, sigint_handler);
  xsignal(SIGUSR1, sigusr1_handler);
  raise(SIGINT);
  assert(sigusr1_handled);

  /* Restore original behavior. */
  xsignal(SIGINT, SIG_DFL);
  xsignal(SIGUSR1, SIG_DFL);

  return 0;
}

/* ======= signal_send ======= */
static void sigusr2_handler(int signo) {
  debug("Child process handles sigusr2.");
  raise(SIGABRT); /* Terminate self. */
}

/* Test sending a signal to a different thread. */
TEST_ADD(signal_send, 0) {
  /* The child should inherit signal handler configuration. */
  xsignal(SIGUSR2, sigusr2_handler);
  pid_t pid = xfork();
  if (pid == 0) {
    debug("This is child (mypid = %d)", getpid());
    /* Wait for signal. */
    while (1)
      sched_yield();
  }

  debug("This is parent (childpid = %d, mypid = %d)", pid, getpid());
  xkill(pid, SIGUSR2);
  wait_child_terminated(pid, SIGABRT);
  debug("Child was terminated by SIGABRT.");
  return 0;
}

/* ======= signal_abort ======= */
/* This test shall be considered success if the process gets terminated with
   SIGABRT */
TEST_ADD(signal_abort, 0) {
  siginfo_t si;
  EXPECT_SIGNAL(SIGABRT, &si) {
    raise(SIGABRT);
  }
  assert(si.si_signo == SIGABRT);
  CLEANUP_SIGNAL();
  return 0;
}

/* ======= signal_segfault ======= */
TEST_ADD(signal_segfault, 0) {
  volatile struct { int x; } *ptr = 0x0;

  siginfo_t si;
  EXPECT_SIGNAL(SIGSEGV, &si) {
    ptr->x = 42;
  }
  CLEANUP_SIGNAL();
  CHECK_SIGSEGV(&si, ptr, SEGV_MAPERR);
  return 0;
}

/* ======= signal_stop ======= */
static volatile int sigcont_handled = 0;
static void sigcont_handler(int signo) {
  sigcont_handled = 1;
}

static volatile int ppid;
static void signal_parent(int signo) {
  xkill(ppid, SIGCONT);
}

TEST_ADD(signal_stop, 0) {
  ppid = getpid();
  xsignal(SIGUSR1, SIG_IGN);
  xsignal(SIGCONT, sigcont_handler);
  pid_t pid = xfork();
  if (pid == 0) {
    xsignal(SIGUSR1, signal_parent);
    /* The child keeps sending SIGUSR1 to the parent. */
    while (!sigcont_handled)
      xkill(ppid, SIGUSR1);
    return 0;
  }

  xsignal(SIGUSR1, sigusr1_handler);
  /* Wait for the child to start sending signals */
  while (!sigusr1_handled)
    sched_yield();
  xkill(pid, SIGSTOP);
  /* Wait for the child to stop. */
  wait_child_stopped(pid);
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
  xkill(pid, SIGUSR1);
  for (int i = 0; i < 3; i++)
    sched_yield();
  assert(!sigcont_handled);
  /* Now continue the child process. */
  xkill(pid, SIGCONT);
  /* The child's SIGUSR1 handler should now run, and so our SIGCONT handler
   * should run too. */
  while (!sigcont_handled)
    sched_yield();
  /* The child process should exit normally. */
  wait_child_finished(pid);
  return 0;
}

/* ======= signal_cont_masked ======= */
TEST_ADD(signal_cont_masked, 0) {
  ppid = getpid();
  xsignal(SIGCONT, sigcont_handler);
  pid_t pid = xfork();
  if (pid == 0) {
    /* Block SIGCONT. */
    sigset_t mask, old;
    sigemptyset(&mask);
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

  /* Wait for the child to stop. */
  wait_child_stopped(pid);
  xkill(pid, SIGCONT);
  wait_child_finished(pid);

  return 0;
}

/* ======= signal_mask ======= */
TEST_ADD(signal_mask, 0) {
  ppid = getpid();
  xsignal(SIGUSR1, signal_parent);
  xsignal(SIGCONT, sigcont_handler);

  pid_t pid = xfork();
  if (pid == 0) {
    while (!sigcont_handled)
      sched_yield();
    return 0;
  }

  /* Check that the signal bounces properly. */
  xkill(pid, SIGUSR1);
  while (!sigcont_handled)
    sched_yield();

  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGCONT);

  /* Mask the signal and make the child send it to us.
   * The delivery of the signal should be delayed until we unblock it. */
  assert(sigprocmask(SIG_BLOCK, &mask, NULL) == 0);
  sigcont_handled = 0;
  xkill(pid, SIGUSR1);

  /* Wait until we get a signal from the child. */
  sigset_t set;
  do {
    sched_yield();
    sigpending(&set);
  } while (!sigismember(&set, SIGCONT));

  assert(!sigcont_handled);

  /* Unblocking a pending signal should make us handle it immediately. */
  assert(sigprocmask(SIG_UNBLOCK, &mask, NULL) == 0);
  assert(sigcont_handled);

  xkill(pid, SIGCONT);

  wait_child_finished(pid);
  return 0;
}

/* ======= signal_mask_nonmaskable ======= */
TEST_ADD(signal_mask_nonmaskable, 0) {
  sigset_t set, old;
  sigemptyset(&set);
  sigaddset(&set, SIGSTOP);
  sigaddset(&set, SIGKILL);
  sigaddset(&set, SIGUSR1);
  /* The call should succeed, but SIGKILL and SIGSTOP shouldn't be blocked. */
  assert(sigprocmask(SIG_BLOCK, &set, &old) == 0);
  assert(sigprocmask(SIG_BLOCK, NULL, &set) == 0);
  sigaddset(&old, SIGUSR1);
  assert(sigsetequal(&set, &old));
  return 0;
}

/* ======= signal_sigsuspend ======= */
TEST_ADD(signal_sigsuspend, 0) {
  pid_t ppid = getpid();
  xsignal(SIGCONT, sigcont_handler);
  xsignal(SIGUSR1, sigusr1_handler);
  sigset_t set, old;
  sigemptyset(&set);
  sigaddset(&set, SIGCONT);
  sigaddset(&set, SIGUSR1);
  assert(sigprocmask(SIG_BLOCK, &set, &old) == 0);
  sigaddset(&old, SIGCONT);
  pid_t cpid = xfork();
  if (cpid == 0) {
    for (int i = 0; i < 10; i++) {
      xkill(ppid, SIGCONT);
      sched_yield();
    }
    xkill(ppid, SIGUSR1);
    return 0;
  }
  /* Go to sleep with SIGCONT blocked and SIGUSR1 unblocked. */
  debug("Calling sigsuspend()...");
  sigset_t current;
  sigprocmask(SIG_BLOCK, NULL, &current);
  assert(sigismember(&current, SIGUSR1));
  assert(!sigismember(&old, SIGUSR1));
  sigsuspend(&old);
  /* Check if mask is set back after waking up */
  sigprocmask(SIG_BLOCK, NULL, &set);
  assert(sigsetequal(&set, &current));
  /* SIGUSR1 should have woken us up, but SIGCONT should still be pending. */
  assert(sigusr1_handled);
  assert(!sigcont_handled);
  sigemptyset(&set);
  sigaddset(&set, SIGCONT);
  assert(sigprocmask(SIG_UNBLOCK, &set, NULL) == 0);
  assert(sigcont_handled);

  wait_child_finished(cpid);
  return 0;
}

/* ======= signal_sigtimedwait ======= */
static int sigtimedwait_child(__unused void *arg) {
  pid_t ppid = getppid();
  syscall_ok(kill(ppid, SIGUSR1));
  return 0;
}

TEST_ADD(signal_sigtimedwait) {
  syscall_ok(signal(SIGCONT, sigcont_handler));
  sigset_t set, current, waitset;
  sigemptyset(&set);
  sigaddset(&set, SIGUSR1);
  sigaddset(&set, SIGCONT);
  syscall_ok(sigprocmask(SIG_SETMASK, &set, NULL));

  utest_spawn(sigtimedwait_child, NULL);

  siginfo_t info;
  sigemptyset(&waitset);
  sigaddset(&waitset, SIGUSR1);
  assert(sigtimedwait(&waitset, &info, NULL) == SIGUSR1);
  assert(info.si_signo == SIGUSR1);

  syscall_ok(sigprocmask(SIG_BLOCK, NULL, &current));
  assert(sigsetequal(&set, &current));

  utest_child_exited(0);
  return 0;
}

/* ======= signal_sigtimedwait_timeout ======= */
static int sigtimedwait_timeout_child(__unused void *arg) {
  ppid = getppid();
  kill(ppid, SIGUSR1);
  kill(ppid, SIGCONT);
  return 0;
}

TEST_ADD(signal_sigtimedwait_timeout) {
  syscall_ok(signal(SIGCONT, sigcont_handler));
  sigset_t set, waitset;
  sigemptyset(&set);
  sigaddset(&set, SIGUSR1);
  syscall_ok(sigprocmask(SIG_SETMASK, &set, NULL));

  siginfo_t info;
  sigemptyset(&waitset);
  sigaddset(&waitset, SIGUSR1);
  timespec_t timeout = {
    .tv_nsec = -1,
    .tv_sec = -1,
  };

  /* tv_nsec is invalid. */
  syscall_fail(sigtimedwait(&waitset, &info, &timeout), EINVAL);

  /* tv_nsec is valid, but tv_sec < 0. */
  timeout.tv_nsec = 10000000;
  syscall_fail(sigtimedwait(&waitset, &info, &timeout), EAGAIN);

  timeout.tv_sec = 0;
  /* Should timeout. */
  syscall_fail(sigtimedwait(&waitset, &info, &timeout), EAGAIN);

  utest_spawn(sigtimedwait_timeout_child, NULL);

  /* If we handled sigcont, then SIGUSR1 must be pending. */
  while (!sigcont_handled)
    sched_yield();

  /* Should not block, but receive the signal as it is in the pending mask. */
  timeout.tv_nsec = 0;
  assert(sigtimedwait(&waitset, &info, &timeout) == SIGUSR1);
  assert(info.si_signo == SIGUSR1);

  utest_child_exited(0);
  return 0;
}

/* ======= signal_sigtimedwait_intr ====== */
int sigtimedwait_intr_child(void *arg) {
  pid_t ppid = getppid();
  while (!sigcont_handled) {
    kill(ppid, SIGCONT);
  }
  return 0;
}

TEST_ADD(signal_sigtimedwait_intr) {
  syscall_ok(signal(SIGCONT, sigcont_handler));
  sigset_t set, waitset;
  sigemptyset(&set);
  sigaddset(&set, SIGUSR1);
  syscall_ok(sigprocmask(SIG_SETMASK, &set, NULL));

  siginfo_t info;
  sigemptyset(&waitset);
  sigaddset(&waitset, SIGUSR1);

  pid_t cpid = utest_spawn(sigtimedwait_intr_child, NULL);

  syscall_fail(sigtimedwait(&waitset, &info, NULL), EINTR);

  syscall_ok(kill(cpid, SIGCONT));
  utest_child_exited(0);
  return 0;
}

/* ======= signal_sigsuspend_stop ======= */
TEST_ADD(signal_sigsuspend_stop, 0) {
  pid_t ppid = getpid();
  xsignal(SIGUSR1, sigusr1_handler);
  sigset_t set, old;
  sigemptyset(&set);
  sigaddset(&set, SIGUSR1);
  assert(sigprocmask(SIG_BLOCK, &set, &old) == 0);
  pid_t cpid = xfork();
  if (cpid == 0) {
    sigsuspend(&old);
    assert(sigusr1_handled);
    xkill(ppid, SIGUSR1);
    return 0;
  }
  /* Wait for the child to call sigsuspend().
   * Right now there's no 100% effective way to do this. */
  for (int i = 0; i < 3; i++)
    sched_yield();

  /* Stop the child. */
  xkill(cpid, SIGSTOP);
  wait_child_stopped(cpid);

  /* Continue the child. This should not interrupt the child's sigsuspend(). */
  xkill(cpid, SIGCONT);
  /* Give the child a chance to run if it has been resumed
   * (which it shouldn't). */
  for (int i = 0; i < 3; i++)
    sched_yield();

  assert(!sigusr1_handled);

  /* Stop the child again. */
  xkill(cpid, SIGSTOP);
  wait_child_stopped(cpid);

  /* Send SIGUSR1 to the child. Since it's stopped, it should not interrupt
   * the sigsuspend() yet. */
  xkill(cpid, SIGUSR1);
  /* Give the child a chance to run if it has been resumed
   * (which it shouldn't). */
  for (int i = 0; i < 3; i++)
    sched_yield();

  assert(!sigusr1_handled);

  /* Continue the child. Now the SIGUSR1 we sent earlier should interrupt
   * the sigsuspend() call. */
  xkill(cpid, SIGCONT);

  /* Wait for the child to send us SIGUSR1. */
  sigsuspend(&old);

  /* Reap the child. */
  wait_child_finished(cpid);
  return 0;
}

/* ======= signal_handler_mask ======= */
static int handler_success;
static int handler_ran;
static pid_t cpid;

static void yield_handler(int signo) {
  /* Give the child process the signal to send us SIGUSR1 */
  xkill(cpid, SIGUSR1);
  while (!sigcont_handled)
    sched_yield();
  handler_ran = 1;
  if (!sigusr1_handled)
    handler_success = 1;
}

TEST_ADD(signal_handler_mask, 0) {
  pid_t ppid = getpid();
  struct sigaction sa = {.sa_handler = yield_handler, .sa_flags = 0};
  /* Block SIGUSR1 when executing handler for SIGUSR2. */
  sigemptyset(&sa.sa_mask);
  sigaddset(&sa.sa_mask, SIGUSR1);
  assert(sigaction(SIGUSR2, &sa, NULL) == 0);
  xsignal(SIGUSR1, sigusr1_handler);
  xsignal(SIGCONT, sigcont_handler);

  pid_t cpid = xfork();
  if (cpid == 0) {
    xkill(ppid, SIGUSR2);
    /* Wait for the parent to enter the signal handler. */
    while (!sigusr1_handled)
      sched_yield();
    /* Now SIGUSR1 should be blocked in the parent. */
    for (int i = 0; i < 3; i++)
      xkill(ppid, SIGUSR1);
    /* Sending SIGCONT should allow yield_handler() to run to completion. */
    xkill(ppid, SIGCONT);
    return 0;
  }

  while (!handler_ran)
    sched_yield();

  assert(handler_success);
  assert(sigusr1_handled);

  wait_child_finished(cpid);
  return 0;
}
