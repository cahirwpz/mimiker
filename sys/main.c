#define KL_LOG KL_USER
#include <klog.h>
#include <stdc.h>
#include <exec.h>
#include <proc.h>
#include <thread.h>
#include <ktest.h>
#include <kenv.h>

#define SIZE 10

int kmain(void) {
  char *argv[SIZE];
  char *envv[SIZE];
  char *test = kenv_get("test");

  int argc = kenv_get_strv("init", argv, SIZE);
  (void)kenv_get_strv("env", envv, SIZE);

  /* Main kernel thread becomes PID(0) - a god process! */
  (void)proc_create(thread_self(), NULL);

  if (argc) {
    run_program(argv[0], argv, envv);
  } else if (test) {
    ktest_main(test);
  } else {
    /* This is a message to the user, so I intentionally use kprintf instead of
     * log. */
    kprintf("============\n");
    kprintf("No init specified!\n");
    kprintf("Use init=\\\"PROGRAM arguments\\\" to start a user-space init "
            "program or test=TEST to run a kernel test.\n");
    kprintf("============\n");
    return 1;
  }

  return 0;
}
