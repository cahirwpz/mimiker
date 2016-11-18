#include <args.h>
#include <stdc.h>

int main() {
  const char *arg1 = kernel_args_get("arg1");
  const char *arg2 = kernel_args_get("arg2");
  int flag1 = kernel_args_get_flag("flag1");
  int flag2 = kernel_args_get_flag("flag2");
  kprintf("Argument arg1 is %s\n", arg1 ? arg1 : "NOT PRESENT");
  kprintf("Argument arg2 is %s\n", arg2 ? arg2 : "NOT PRESENT");
  kprintf("Flag flag1 is %s\n", flag1 ? "SET" : "NOT SET");
  kprintf("Flag flag2 is %s\n", flag2 ? "SET" : "NOT SET");
  return 0;
}
