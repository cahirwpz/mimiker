#include "utest.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void print_msg(const char *file, int line, const char *fmt, va_list ap) {
  char buf[1024];
  int len = 0, res;

  char *basename = strrchr(file, '/');
  file = basename ? basename + 1 : file;
  res = snprintf_ss(buf, sizeof(buf), "[%s:%d] ", file, line);
  if (res < 0)
    exit(EXIT_FAILURE);
  len += res;

  res = vsnprintf_ss(buf + len, sizeof(buf) - len, fmt, ap);
  if (res < 0)
    exit(EXIT_FAILURE);

  len += res;
  buf[len++] = '\n';
  (void)write(STDERR_FILENO, buf, (size_t)len);
}

int __verbose = 0;

void __msg(const char *file, int line, const char *fmt, ...) {
  if (!__verbose)
    return;

  va_list ap;
  va_start(ap, fmt);
  print_msg(file, line, fmt, ap);
  va_end(ap);
}

noreturn void __die(const char *file, int line, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  print_msg(file, line, fmt, ap);
  va_end(ap);

  exit(EXIT_FAILURE);
}

pid_t spawn(proc_func_t func, void *arg) {
  int pid = xfork();
  if (pid == 0)
    exit(func(arg));
  return pid;
}

pid_t wait_child_exited(pid_t pid, int exitcode) {
  int status;
  pid_t res = xwaitpid(pid, &status, 0);
  if (pid > 0)
    assert(res == pid);
  assert(WIFEXITED(status));
  assert(WEXITSTATUS(status) == exitcode);
  return res;
}

pid_t wait_child_terminated(pid_t pid, int signo) {
  int status;
  pid_t res = xwaitpid(pid, &status, 0);
  if (pid > 0)
    assert(res == pid);
  assert(WIFSIGNALED(status));
  assert(WTERMSIG(status) == signo);
  return res;
}

void wait_child_stopped(pid_t pid) {
  int status;
  assert(xwaitpid(pid, &status, WUNTRACED) == pid);
  assert(WIFSTOPPED(status));
}

void wait_child_continued(pid_t pid) {
  int status;
  assert(xwaitpid(pid, &status, WCONTINUED) == pid);
  assert(WIFCONTINUED(status));
}
