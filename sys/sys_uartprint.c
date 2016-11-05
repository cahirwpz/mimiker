#include <syscall.h>
#include <stdc.h>

void sys_uart_print_char(syscall_args_t *sa) {
  char c = sa->args[0];
  kprintf("%c", c);
}
