#define KL_LOG KL_USER
#include <sys/klog.h>
#include <sys/libkern.h>
#include <sys/kenv.h>
#include <sys/exec.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/ktest.h>

int kmain(void) {
  char *init = kenv_get("init");
  char *test = kenv_get("test");

  /* Main kernel thread becomes PID(1) - a god process! */
  proc_t *init_proc = proc_create(thread_self(), NULL);
  proc_add(init_proc);
  assert(init_proc->p_pid == 1);
  int error = pgrp_enter(init_proc, init_proc->p_pid, true);
  assert(error == 0);

  if (init) {
    run_program(init, kenv_get_init(), (char *[]){NULL});
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
