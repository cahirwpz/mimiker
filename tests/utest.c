#include <common.h>
#include <exec.h>
#include <ktest.h>
#include <thread.h>
#include <sched.h>

static void utest_generic_thread(void *arg) {
  const char *test_name = arg;
  exec_args_t exec_args;
  exec_args.prog_name = "/bin/utest";
  exec_args.argv = (const char *[]){"utest", test_name};
  exec_args.argc = 2;

  do_exec(&exec_args);
  __unreachable();
}

static int utest_generic(const char *name) {
  thread_t *utest_thread =
    thread_create("fd_test", utest_generic_thread, (void *)name);
  sched_add(utest_thread);
  thread_join(utest_thread);
  int retval = utest_thread->td_exitcode;
  if (retval == 0)
    return KTEST_SUCCESS;
  else {
    kprintf("User test %s failed, exitcode: %d\n", name, retval);
    return KTEST_FAILURE;
  }
}

/* Adds a new test executed by running /bin/utest. */
#define UTEST_ADD(name, flags)                                                 \
  static int utest_test_##name() {                                             \
    return utest_generic(#name);                                               \
  }                                                                            \
  KTEST_ADD(user_##name, utest_test_##name, flags | KTEST_FLAG_USERMODE);

UTEST_ADD(mmap, 0);
UTEST_ADD(sbrk, 0);
UTEST_ADD(misbehave, 0);

UTEST_ADD(fd_read, 0);
UTEST_ADD(fd_devnull, 0);
UTEST_ADD(fd_multidesc, 0);
UTEST_ADD(fd_readwrite, 0);
UTEST_ADD(fd_copy, 0);
UTEST_ADD(fd_bad_desc, 0);
UTEST_ADD(fd_open_path, 0);
UTEST_ADD(fd_dup, 0);
UTEST_ADD(fd_all, 0);
