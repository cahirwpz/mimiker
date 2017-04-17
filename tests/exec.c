#include <common.h>
#include <exec.h>
#include <thread.h>
#include <sched.h>
#include <ktest.h>

static void program_thread(void *data) {
  exec_args_t exec_args;
  switch ((int)data) {
    case 1:
      exec_args.prog_name = "/bin/prog";
      exec_args.argv =
        (const char *[]){"prog", "argument1", "ARGUMENT2", "a-r-g-u-m-e-n-t-3"};
      exec_args.argc = 4;
      do_exec(&exec_args);
    case 2:
      exec_args.prog_name = "/bin/prog";
      exec_args.argv = (const char *[]){"prog", "String passed as argument."};
      exec_args.argc = 2;
      do_exec(&exec_args);
    case 3:
      exec_args.prog_name = "/bin/prog";
      exec_args.argv = (const char *[]){"prog", "abort_test"};
      exec_args.argc = 2;
      do_exec(&exec_args);
  }
}

static int test_exec() {
  /* This is a simple demonstration of the exec functionality. It
   * requests to substitute current thread's image with program
   * called "prog", which is implemented in ./user/prog.c, and after
   * compilation stored in kernel ramdisk.

   * As the loaded program has no means to communicate with the
   * system, because system calls are not yet implemented, it runs
   * quietly. To test its behavior with a debugger, after starting
   * gdb issue the command:
   *    add-symbol-file user/prog.uelf 0x00400000
   * which will load the symbols for the user program. You may then
   * e.g. break at its main(), and see that argc and argv are set
   * correctly. If you let it run further, you may `print textarea`
   * to see that it accesses a string in .data and copies it to
   * .bss.
   */

  thread_t *td1 = thread_create("user_thread1", program_thread, (void *)1);
  thread_t *td2 = thread_create("user_thread2", program_thread, (void *)2);
  thread_t *td3 = thread_create("user_thread3", program_thread, (void *)3);

  sched_add(td1);
  sched_add(td2);
  sched_add(td3);

  sched_run();

  return KTEST_FAILURE;
}

KTEST_ADD(exec, test_exec, KTEST_FLAG_NORETURN | KTEST_FLAG_USERMODE);
