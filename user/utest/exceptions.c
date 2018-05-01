#include "utest.h"

#include <assert.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <stdlib.h>

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

static inline void exec_cp0_instr(void) {
  int value;
  asm volatile("mfc0 %0, $12, 0" : "=r" (value));
}

static inline void exec_reserved_instr(void) {
  /* The choice of reserved opcode was done based on "Table A.2 MIPS32 Encoding
   * of the Opcode Field" from "MIPS® Architecture For Programmers Volume II-A:
   * The MIPS32® Instruction Set" */
  asm volatile(".long 0x00000039"); /* objdump fails to disassemble it */
}

static int check_exception(void (*code)(void), int signum) {
  spawn_process(code);
  int status;
  wait(&status);
  assert(WIFSIGNALED(status));
  assert(signum == WTERMSIG(status));
  return 0;
}

int test_exc_cop_unusable(void) {
  return check_exception(exec_cp0_instr, SIGILL);
}

int test_exc_reserved_instruction(void) {
  return check_exception(exec_reserved_instr, SIGILL);
}

int test_syscall_in_bds(void) {
  unsigned control = 1;
  char *text = "write executed\n";

  /* executing write(1, text, sizeof(text)) */
  asm volatile(".set noreorder;"
               "li $a0, 1;"
               "move $a1, %1;"
               "move $a2, %2;"
               "li $v0, 5;"
               "j 1f;"
               "syscall;"
               "sw $zero, %0;"
               "1:;"
               : "=m" (control)
               : "r" (text), "r" (sizeof(text))
               : "a0", "a1", "a2", "v0", "memory");

  assert(control == 1);
  return 0;
}
