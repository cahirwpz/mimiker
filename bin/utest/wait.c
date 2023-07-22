#include "utest.h"

#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

static int nothing_to_report(pid_t pid) {
  return xwaitpid(pid, NULL, WCONTINUED | WUNTRACED | WNOHANG) == 0;
}

/* ======= wait_basic ======= */
static volatile int sigcont_handled = 0;
static volatile int ppid;
static void sigcont_handler(int signo) {
  sigcont_handled = 1;
}

TEST_ADD(wait_basic, 0) {
  ppid = getpid();
  xsignal(SIGCONT, sigcont_handler);
  pid_t pid = xfork();
  if (pid == 0) {
    while (!sigcont_handled)
      sched_yield();
    sigcont_handled = 0;
    xkill(ppid, SIGCONT);
    while (!sigcont_handled)
      sched_yield();
    return 0;
  }

  /* Nothing has happened yet, so waitpid shouldn't report anything. */
  assert(nothing_to_report(pid));

  xkill(pid, SIGSTOP);
  wait_child_stopped(pid);

  assert(nothing_to_report(pid));

  xkill(pid, SIGCONT);
  wait_child_continued(pid);

  assert(nothing_to_report(pid));

  /* Wait until the child is ready to accept the second SIGCONT. */
  while (!sigcont_handled)
    sched_yield();

  xkill(pid, SIGCONT);
  wait_child_finished(pid);
  return 0;
}

/* ======= wait_nohang ======= */
TEST_ADD(wait_nohang, 0) {
  ppid = getpid();
  xsignal(SIGCONT, sigcont_handler);
  pid_t pid = xfork();
  if (pid == 0) {
    while (!sigcont_handled)
      sched_yield();
    sigcont_handled = 0;
    xkill(ppid, SIGCONT);
    while (!sigcont_handled)
      sched_yield();
    return 0;
  }

  int status;
  /* Nothing has happened yet, so waitpid shouldn't report anything. */
  assert(nothing_to_report(pid));

  xkill(pid, SIGSTOP);
  while (xwaitpid(pid, &status, WUNTRACED | WNOHANG) != pid)
    sched_yield();
  assert(WIFSTOPPED(status));

  assert(nothing_to_report(pid));

  xkill(pid, SIGCONT);
  while (xwaitpid(pid, &status, WCONTINUED | WNOHANG) != pid)
    sched_yield();
  assert(WIFCONTINUED(status));

  assert(nothing_to_report(pid));

  /* Wait until the child is ready to accept the second SIGCONT. */
  while (!sigcont_handled)
    sched_yield();

  xkill(pid, SIGCONT);
  while (xwaitpid(pid, &status, WNOHANG) != pid)
    sched_yield();
  assert(WIFEXITED(status));
  assert(WEXITSTATUS(status) == 0);
  return 0;
}
