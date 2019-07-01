#define KL_LOG KL_USER
#include <klog.h>
#include <stdc.h>
#include <exec.h>
#include <proc.h>
#include <thread.h>
#include <ktest.h>

/* Borrowed from mips/malta.c */
char *kenv_get(const char *key);

int kmain(void) {
  char *init = kenv_get("init");
  char *test = kenv_get("test");

  /* Main kernel thread becomes PID(0) - a god process! */
  proc_add(proc_create(thread_self(), NULL));

  if (init) {
    run_program(init, (char *[]){init, NULL}, (char *[]){NULL});
  } else if (test) {
    ktest_main(test);
  } else {
    /* This is a message to the user, so I intentionally use kprintf instead of
     * log. */
    kprintf("============\n");
    kprintf("No init specified!\n");
    kprintf("Use init=PROGRAM to start a user-space init program or test=TEST "
            "to run a kernel test.\n");
    kprintf("============\n");
    return 1;
  }

  return 0;
}
