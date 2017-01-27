#include <common.h>
#include <exec.h>
#include <ktest.h>

static int test_exec_misbehave() {
  exec_args_t exec_args;
  exec_args.prog_name = "misbehave";
  exec_args.argv = (const char *[]){"misbehave"};
  exec_args.argc = 1;

  do_exec(&exec_args);

  return 0;
}

KTEST_ADD(exec_misbehave, test_exec_misbehave,
                KTEST_FLAG_NORETURN | KTEST_FLAG_USERMODE);
