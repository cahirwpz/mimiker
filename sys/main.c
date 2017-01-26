#include <stdc.h>
#include <exec.h>
#include <ktest.h>
#include <malloc.h>

/* Borrowed from mips/malta.c */
char *kenv_get(const char *key);

static void run_init(const char *program) {
  log("Starting program \"%s\"", program);

  exec_args_t exec_args;
  exec_args.prog_name = program;
  exec_args.argv = (const char *[]){program};
  exec_args.argc = 1;

  int res = do_exec(&exec_args);
  if (res) {
    log("Failed to start init program.");
  }
}

int main() {
  const char *init = kenv_get("init");
  const char *test = kenv_get("test");

  if (init)
    run_init(init);
  else if (test) {
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
