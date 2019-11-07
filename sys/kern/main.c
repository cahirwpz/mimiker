#define KL_LOG KL_USER
#include <sys/klog.h>
#include <sys/libkern.h>
#include <sys/exec.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/ktest.h>
#include <sys/malloc.h>

/* Borrowed from mips/malta.c */
char *kenv_get(const char *key);
char **kenv_get_init_args(int *n);

int kmain(void) {
  char *init = kenv_get("init");
  char *test = kenv_get("test");

  /* Main kernel thread becomes PID(0) - a god process! */
  proc_add(proc_create(thread_self(), NULL));

  if (init) {
    int ninit_args;
    char **init_args = kenv_get_init_args(&ninit_args);

    if (init_args) {
      char **args = kmalloc(M_TEMP, sizeof(char *) * (ninit_args + 2), 0);
      args[0] = init;
      for (int i = 0; i < ninit_args; ++i) {
        args[i + 1] = init_args[i];
      }
      args[ninit_args + 1] = NULL;
      run_program(init, args, (char *[]){NULL});
    } else {
      run_program(init, (char *[]){init, NULL}, (char *[]){NULL});
    }
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
