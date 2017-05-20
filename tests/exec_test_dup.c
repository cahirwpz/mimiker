#include <common.h>
#include <exec.h>
#include <ktest.h>
#include <thread.h>
#include <sched.h>

static void test_dup_user_thread(void *arg) {
  exec_args_t exec_args;
  exec_args.prog_name = "/bin/test_dup";
  exec_args.argv = (const char *[]){"test_dup"};
  exec_args.argc = 1;

  do_exec(&exec_args);
}

static int test_exec_test_dup() {
  thread_t *user_thread = thread_create("test_dup", test_dup_user_thread, NULL);
  sched_add(user_thread);
  thread_join(user_thread);
  /* TODO: Check *process* exit code. */
  /* assert(user_thread->td_exitcode == 0); */
  return KTEST_SUCCESS;
}

KTEST_ADD(exec_test_dup, test_exec_test_dup, KTEST_FLAG_USERMODE);
