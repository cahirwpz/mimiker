#include "utest.h"

#include <assert.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <stdlib.h>

static inline void exec_cp0_instr(void) {
  int value;
  asm volatile("mfc0 %0, $12, 0" : "=r"(value));
}

static inline void exec_reserved_instr(void) {
  int value;
  asm volatile("mfc2 %0, $12, 0" : "=r"(value));
}

static int spawn_process(void (*proc_handler)(void)) {
  int pid;
  switch ((pid = fork())) {
    case -1:
      exit(EXIT_FAILURE);
    case 0:
      proc_handler();
      exit(EXIT_SUCCESS);
    default:
      return pid;
  }
}

static void check_exception(void (*code)(void), int signum) {
  spawn_process(code);
  int status;
  wait(&status);
  assert(WIFSIGNALED(status));
  assert(signum == WTERMSIG(status));
}

int test_userspace_cop_unusable(void) {
  check_exception(exec_cp0_instr, SIGILL);
  return 0;
}

int test_reserved_instruction(void) {
  check_exception(exec_reserved_instr, SIGILL);
  return 0;
}