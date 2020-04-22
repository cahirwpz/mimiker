#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sched.h>
#include <sys/wait.h>

#include "utest.h"

/* ======= wait_basic ======= */
static volatile int sigcont_handled = 0;
static volatile int ppid;
static void sigcont_handler(int signo) {
  sigcont_handled = 1;
}
int test_wait_basic() {
  ppid = getpid();
  signal(SIGCONT, sigcont_handler);
  int pid = fork();
  if (pid == 0) {
    while (!sigcont_handled)
      sched_yield();
    sigcont_handled = 0;
    kill(ppid, SIGCONT);
    while (!sigcont_handled)
      sched_yield();
    return 0;
  }

  int status;
  /* Nothing has happened yet, so waitpid shouldn't report anything. */
  assert(waitpid(pid, NULL, WCONTINUED | WUNTRACED | WNOHANG) == 0);

  kill(pid, SIGSTOP);
  assert(waitpid(pid, &status, WUNTRACED) == pid);
  assert(WIFSTOPPED(status));

  assert(waitpid(pid, NULL, WCONTINUED | WUNTRACED | WNOHANG) == 0);

  kill(pid, SIGCONT);
  assert(waitpid(pid, &status, WCONTINUED) == pid);
  assert(WIFCONTINUED(status));

  assert(waitpid(pid, NULL, WCONTINUED | WUNTRACED | WNOHANG) == 0);

  /* Wait until the child is ready to accept the second SIGCONT. */
  while (!sigcont_handled)
    sched_yield();

  kill(pid, SIGCONT);
  assert(waitpid(pid, &status, 0) == pid);
  assert(WIFEXITED(status));
  assert(WEXITSTATUS(status) == 0);
  return 0;
}

/* ======= wait_nohang ======= */
int test_wait_nohang() {
  ppid = getpid();
  signal(SIGCONT, sigcont_handler);
  int pid = fork();
  if (pid == 0) {
    while (!sigcont_handled)
      sched_yield();
    sigcont_handled = 0;
    kill(ppid, SIGCONT);
    while (!sigcont_handled)
      sched_yield();
    return 0;
  }

  int status, rv;
  /* Nothing has happened yet, so waitpid shouldn't report anything. */
  assert(waitpid(pid, NULL, WCONTINUED | WUNTRACED | WNOHANG) == 0);

  kill(pid, SIGSTOP);
  while ((rv = waitpid(pid, &status, WUNTRACED | WNOHANG)) != pid) {
    assert(rv == 0);
    sched_yield();
  }
  assert(WIFSTOPPED(status));

  assert(waitpid(pid, NULL, WCONTINUED | WUNTRACED | WNOHANG) == 0);

  kill(pid, SIGCONT);
  while ((rv = waitpid(pid, &status, WCONTINUED | WNOHANG)) != pid) {
    assert(rv == 0);
    sched_yield();
  }
  assert(WIFCONTINUED(status));

  assert(waitpid(pid, NULL, WCONTINUED | WUNTRACED | WNOHANG) == 0);

  /* Wait until the child is ready to accept the second SIGCONT. */
  while (!sigcont_handled)
    sched_yield();

  kill(pid, SIGCONT);
  while ((rv = waitpid(pid, &status, WNOHANG)) != pid) {
    assert(rv == 0);
    sched_yield();
  }
  assert(WIFEXITED(status));
  assert(WEXITSTATUS(status) == 0);
  return 0;
}
