#include <common.h>
#include <exec.h>
#include <ktest.h>

static int test_exec_fd_test() {
  exec_args_t exec_args;
  exec_args.prog_name = "fd_test";
  exec_args.argv = (const char *[]){"fd_test"};
  exec_args.argc = 1;

  do_exec(&exec_args);

  return KTEST_FAILURE;
}

KTEST_ADD(exec_fd_test, test_exec_fd_test,
                KTEST_FLAG_NORETURN | KTEST_FLAG_USERMODE);
