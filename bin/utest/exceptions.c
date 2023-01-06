#include "utest.h"
#include "util.h"

#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>
#include <sys/wait.h>

#ifdef __mips__

TEST_ADD(exc_cop_unusable) {
  siginfo_t si;
  EXPECT_SIGNAL(SIGILL, &si) {
    int value;
    asm volatile("mfc0 %0, $12, 0" : "=r"(value));
  }
  CLEANUP_SIGNAL();
  return 0;
}

TEST_ADD(exc_reserved_instruction) {
  /* The choice of reserved opcode was done based on "Table A.2 MIPS32 Encoding
   * of the Opcode Field" from "MIPS® Architecture For Programmers Volume II-A:
   * The MIPS32® Instruction Set" */
  siginfo_t si;
  EXPECT_SIGNAL(SIGILL, &si) {
    asm volatile(".long 0x00000039"); /* objdump fails to disassemble it */
  }
  CLEANUP_SIGNAL();
  return 0;
}

TEST_ADD(exc_integer_overflow) {
  siginfo_t si;
  EXPECT_SIGNAL(SIGFPE, &si) {
    int d = __INT_MAX__;
    asm volatile("addi %0, %0, 1" : : "r"(d));
  }
  CLEANUP_SIGNAL();
  return 0;
}

TEST_ADD(exc_unaligned_access) {
  siginfo_t si;
  EXPECT_SIGNAL(SIGBUS, &si) {
    int a[2];
    int val;
    asm volatile("lw %0, 1(%1)" : "=r"(val) : "r"(a));
  }
  CLEANUP_SIGNAL();
  return 0;
}

TEST_ADD(syscall_in_bds) {
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
               : "=m"(control)
               : "r"(text), "r"(sizeof(text))
               : "a0", "a1", "a2", "v0", "memory");

  assert(control == 1);
  return 0;
}

#endif /* !__mips__ */

#ifdef __aarch64__

TEST_ADD(exc_unknown_instruction) {
  siginfo_t si;
  EXPECT_SIGNAL(SIGILL, &si) {
    asm volatile(".long 0x00110011");
  }
  CLEANUP_SIGNAL();
  return 0;
}

TEST_ADD(exc_msr_instruction) {
  siginfo_t si;
  EXPECT_SIGNAL(SIGILL, &si) {
    asm volatile("msr spsr_el1, %0" ::"r"(1ULL));
  }
  CLEANUP_SIGNAL();
  return 0;
}

TEST_ADD(exc_mrs_instruction) {
  siginfo_t si;
  EXPECT_SIGNAL(SIGILL, &si) {
    long x;
    asm volatile("mrs %0, spsr_el1" : "=r"(x));
  }
  CLEANUP_SIGNAL();
  return 0;
}

TEST_ADD(exc_brk) {
  siginfo_t si;
  EXPECT_SIGNAL(SIGTRAP, &si) {
    asm volatile("brk 0");
  }
  CLEANUP_SIGNAL();
  return 0;
}

#endif /* !__aarch64__ */
