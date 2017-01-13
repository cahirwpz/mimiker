#include <common.h>
#include <exec.h>
#include <test.h>

int test_exec_misbehave() {
  exec_args_t exec_args;
  exec_args.prog_name = "misbehave";
  exec_args.argv = (const char *[]){"misbehave"};
  exec_args.argc = 1;

  do_exec(&exec_args);

  return 0;
}

TEST_ADD(exec_misbehave, test_exec_misbehave);
