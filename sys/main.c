#include <stdc.h>
#include <exec.h>
#include <ktest.h>

/* Borrowed from mips/malta.c */
char *kenv_get(const char *key);

/* Stores currently running test data. */
static test_entry_t *current_test = NULL;

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

static test_entry_t *find_test_by_name(const char *test) {
  SET_DECLARE(tests, test_entry_t);
  test_entry_t **ptr;
  SET_FOREACH(ptr, tests) {
    if (strcmp((*ptr)->test_name, test) == 0) {
      return *ptr;
    }
  }
  return NULL;
}

static void run_test(test_entry_t *t) {
  /* These are messages to the user, so I intentionally use kprintf instead of
   * log. */
  kprintf("Running test %s.\n", t->test_name);
  if (t->flags & KTEST_FLAG_NORETURN)
    kprintf("WARNING: This test will never return, it is not possible to "
            "automatically verify its success.\n");
  if (t->flags & KTEST_FLAG_USERMODE)
    kprintf("WARNING: This test will enters usermode.\n");
  if (t->flags & KTEST_FLAG_DIRTY)
    kprintf("WARNING: This test will break kernel state. Kernel reboot will be "
            "required to run any other test.\n");

  current_test = t;
  int result = t->test_func();
  current_test = NULL;

  if (result == KTEST_SUCCESS)
    kprintf(TEST_PASSED_STRING);
  else
    kprintf(TEST_FAILED_STRING);
}

int main() {
  const char *init = kenv_get("init");
  const char *test = kenv_get("test");

  if (init)
    run_init(init);
  else if (test) {
    test_entry_t *t = find_test_by_name(test);
    if (!t) {
      kprintf("Test \"%s\" not found!", test);
      return 1;
    }
    run_test(t);
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
