#include "utest.h"

#include <assert.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <stdlib.h>

#include <stdio.h>

#ifdef __mips__

int test_exc_cop_unusable(void) {
  int value;
  asm volatile("mfc0 %0, $12, 0" : "=r"(value));
  return 0;
}

int test_exc_reserved_instruction(void) {
  /* The choice of reserved opcode was done based on "Table A.2 MIPS32 Encoding
   * of the Opcode Field" from "MIPS® Architecture For Programmers Volume II-A:
   * The MIPS32® Instruction Set" */
  asm volatile(".long 0x00000039"); /* objdump fails to disassemble it */
  return 0;
}

int test_exc_integer_overflow(void) {
  int d = __INT_MAX__;
  asm volatile("addi %0, %0, 1" : : "r"(d));

  return 0;
}

int test_exc_unaligned_access(void) {
  int a[2];
  int val;
  asm volatile("lw %0, 1(%1)" : "=r"(val) : "r"(a));

  return 0;
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
               : "=m"(control)
               : "r"(text), "r"(sizeof(text))
               : "a0", "a1", "a2", "v0", "memory");

  assert(control == 1);
  return 0;
}

#endif /* !__mips__ */
