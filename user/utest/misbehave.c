#include "utest.h"

#include <stddef.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <stdlib.h>

#define MFC0(var, reg, sel) asm volatile("mfc0 %0," #reg "," #sel : "=r"(var))

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

static int spawn_sprocess_calling_cp0(void) {
  int pid;
  // int value;
  switch((pid = fork())) {
    case -1:
      exit(1);
    case 0:
      // MFC0(value, $12, 0);
      // asm volatile("mfc0 %0,$12,0" : "=r"(value));
      raise(SIGILL);
      exit(1);
    default : 
      return pid;
  }
}

int test_call_cp0_from_userspace() {
  spawn_sprocess_calling_cp0();
  int status;
  wait(&status);
  assert(WIFSIGNALED(status));
  assert(SIGILL == WTERMSIG(status));
  return 0;
}