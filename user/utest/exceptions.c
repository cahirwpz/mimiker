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
    asm volatile(".long 1337" :);
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

int test_syscall_in_bds(void) {

  unsigned int control = 0;
  char *text = "write executed\n";
  int text_len = 16;

  // executing write(1, text, count)
  asm volatile(".set noreorder;"
               "addiu $a0, $zero, 1;"
               "move $a2, %2;"
               "move $a1, %1;"
               "addiu $v0, $zero, 5;"
               "j 1f;"
               "syscall;"
               "lw $1, %0;"
               "ori $1, $1, 1;"
               "sw $1, %0;"
               "1:;"
               : "=m"(control)
               : "r"(text), "r"(text_len)
               : "a0", "a1", "a2", "v0", "1", "memory");

  assert(control == 0);
  return 0;
}
