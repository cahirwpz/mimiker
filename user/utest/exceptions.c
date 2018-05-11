#include "utest.h"

#include <assert.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <stdlib.h>

#include <stdio.h>

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

// TODO not working
int test_exc_fpe(void){
  // #pragma GCC diagnostic push
  // #pragma GCC diagnostic ignored "-Wdiv-by-zero"
  float a = 0.0;
  float b = 1.0;
  float c = b / a;
  (void)c;
  // #pragma GCC diagnostic pop
  return 0;
}

int test_exc_unaligned_access(void){
  int a[2];
  asm volatile ("addiu $a0, %0, 1;"
                "lw $a1, 0($a0)" 
                : 
                : "r"(a)
                : "a0", "a1");

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
