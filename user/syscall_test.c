
void syscall_uart_print_char(char c) {
  asm volatile("move $a0, %0\n"
               "move $v0, 1001\n"
               "syscall"
               :
               : "r"(c));
}

int main(int argc, char **argv) {
  const char *str = "Hello world from a user program!\n";
  const char *c = str;
  while (*c)
    syscall_uart_print_char(*c);
  while (1)
    ;
}
