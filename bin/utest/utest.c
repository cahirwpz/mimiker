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

int utest_spawn(proc_func_t func, void *arg) {
  int pid;
  switch ((pid = fork())) {
    case -1:
      exit(EXIT_FAILURE);
    case 0:
      exit(func(arg));
    default:
      return pid;
  }
}

void utest_child_exited(int exitcode) {
  int status;
  wait(&status);
  assert(WIFEXITED(status));
  assert(WEXITSTATUS(status) == exitcode);
}
