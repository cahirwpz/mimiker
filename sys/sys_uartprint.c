#include <syscall.h>
#include <stdc.h>

void sys_uart_print_char(syscall_args_t *sa) {
  char c = sa->args[0];
  kprintf("%c", c);
}

void sys_uart_print_str(syscall_args_t *sa) {
  /* TODO: Copy string from user space? Is that even necessary for
     this case? */
  char *c = (char *)sa->args[0];
  kprintf("%s", c);
}
