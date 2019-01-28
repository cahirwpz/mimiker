#define KL_LOG KL_USER
#include <klog.h>
#include <stdc.h>
#include <exec.h>
#include <proc.h>
#include <thread.h>
#include <ktest.h>
#include <kenv.h>

int kmain(int argc, char **argv) {
  char **init = kenv_get_user_argv();
  char *test = kenv_get("test");

  /* Main kernel thread becomes PID(0) - a god process! */
  (void)proc_create(thread_self(), NULL);

  if (init) {
    run_program(*init, init, kenv_get_user_envv());
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
