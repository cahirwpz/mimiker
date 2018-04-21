#include "utest.h"

#include <stddef.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/signal.h>

int test_misbehave() {
  const char str[] = "Hello world from a user program!\n";

  /* XXX: Currently kernel does not sigsegv offending programs, but in future it
     will, so this test will behave differently. */

  /* Successful syscall */
  assert(write(STDOUT_FILENO, str, sizeof(str) - 1) == sizeof(str) - 1);

  /* Pass bad pointer (kernel space) */
  assert(write(STDOUT_FILENO, (char *)0xDEADC0DE, 100) == -1);
  assert(errno == EFAULT);

  /* Pass bad pointer (user space) */
  assert(write(STDOUT_FILENO, (char *)0x1EE7C0DE, 100) == -1);
  assert(errno == EFAULT);

  return 0;
}

static inline void exec_cp0_instr(void){
  int value;
  asm volatile("mfc0 %0, $12, 0" : "=r"(value));
}

static inline void exec_reserved_instr(void) {
  // how to raise Reserved Instruction exception?
  // int value;
  // asm volatile("mfc0 %0, $12, 0" : "=r"(value));
}

static int spawn_process(void (*proc_handler)(void)) {
  int pid;
  switch ((pid = fork())) {
    case -1 : 
      exit(EXIT_FAILURE);
    case 0 : 
      proc_handler();
      exit(EXIT_SUCCESS);
    default : 
      return pid;
  }
}

int test_userspace_cop_unusable(void) {
  spawn_process(exec_cp0_instr);
  int status;
  wait(&status);
  assert(WIFSIGNALED(status));
  assert(SIGILL == WTERMSIG(status));
  return 0;
}

int test_reserved_instruction(void) {
  spawn_process(exec_reserved_instr);
  int status;
  wait(&status);
  assert(WIFSIGNALED(status));
  assert(SIGILL == WTERMSIG(status));
  return 0;
}