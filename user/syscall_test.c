
void syscall_uart_print_char(char c) {
  asm volatile("move $a0, %0\n"
               "li $v0, 1001\n"
               "syscall"
               :
               : "r"(c));
}

void syscall_uart_print_str(const char *c) {
  asm volatile("move $a0, %0\n"
               "li $v0, 1002\n"
               "syscall"
               :
               : "r"(c));
}

int main(int argc, char **argv) {
  /* Char-by-char output */
  const char *str = "Hello world from a user program!\n";
  const char *c = str;
  while (*c)
    syscall_uart_print_char(*c++);
  /* String output */
  syscall_uart_print_str(str);
  while (1)
    ;
}
