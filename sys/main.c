#include <stdc.h>
#include <exec.h>
#include <ktest.h>

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

static void run_test(const char *test) {
  log("Running test \"%s\"", test);
  SET_DECLARE(tests, test_entry_t);
  test_entry_t **ptr;
  SET_FOREACH(ptr, tests) {
    if (strcmp((*ptr)->test_name, test) == 0) {
      (*ptr)->test_func();
      return;
    }
  }
  log("Test \"%s\" not found!", test);
}

int main() {
  const char *init = kenv_get("init");
  const char *test = kenv_get("test");

  if (init)
    run_init(init);
  else if (test)
    run_test(test);
  else {
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
