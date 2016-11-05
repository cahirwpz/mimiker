#include <common.h>
#include <exec.h>

int main() {
  exec_args_t exec_args;
  exec_args.prog_name = "syscall_test";
  exec_args.argv = (char *[]){};
  exec_args.argc = 0;

  do_exec(&exec_args);

  return 0;
}
