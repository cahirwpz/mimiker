#include <common.h>
#include <exec.h>
#include <test.h>

int test_exec_fd_test() {
  exec_args_t exec_args;
  exec_args.prog_name = "fd_test";
  exec_args.argv = (const char *[]){"fd_test"};
  exec_args.argc = 1;

  do_exec(&exec_args);

  return 0;
}

TEST_ADD(exec_fd_test, test_exec_fd_test);
