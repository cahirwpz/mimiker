#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <stdint.h>
#include <mips/mips.h>

#define SYSCALL_ARGS_MAX 4

/* Low syscall codes reserved for actual ABI. */
#define SYS_UARTPRINT_CHAR 1001
#define SYS_UARTPRINT_STR 1002

typedef struct syscall_args {
  uint32_t code;
  reg_t args[SYSCALL_ARGS_MAX];
} syscall_args_t;

void sys_uart_print_char(syscall_args_t *sa);
void sys_uart_print_str(syscall_args_t *sa);

#endif // __SYSCALL_H__
