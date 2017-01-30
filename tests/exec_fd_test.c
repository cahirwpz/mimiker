#include <common.h>
#include <exec.h>
#include <ktest.h>
#include <thread.h>
#include <sched.h>

static void test_fd_user_thread(void *arg) {
  exec_args_t exec_args;
  exec_args.prog_name = "fd_test";
  exec_args.argv = (const char *[]){"fd_test"};
  exec_args.argc = 1;

  do_exec(&exec_args);
}

static int test_exec_fd_test() {
  thread_t *user_thread = thread_create("fd_test", test_fd_user_thread, NULL);
  sched_add(user_thread);
  ktest_wait_for_user_thread(user_thread, 1000);
  return KTEST_SUCCESS;
}

KTEST_ADD(exec_fd_test, test_exec_fd_test, KTEST_FLAG_USERMODE);
