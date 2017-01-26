#include <stdc.h>
#include <exec.h>
#include <ktest.h>
#include <malloc.h>

/* Borrowed from mips/malta.c */
char *kenv_get(const char *key);

/* Stores currently running test data. */
static test_entry_t *current_test = NULL;
/* A null-terminated array of pointers to the tested test list. */
static test_entry_t **autorun_tests;

/* We only need this pool to allocate some memory for performing operations on
 * the list of all tests. */
static MALLOC_DEFINE(test_pool, "test data pool");

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

void ktest_failure() {
  if (current_test == NULL)
    panic("current_test == NULL in ktest_failure! This is most likely a bug in "
          "the test framework!\n");
  kprintf(TEST_FAILED_STRING);
  if (autorun_tests) {
    kprintf("Failure while running multiple tests.\n");
  } else {
    kprintf("Failure while running single test.\n");
    kprintf("Failing test: %s\n", current_test->test_name);
  }
  panic("Halting kernel on failed test.\n");
}

static int run_test(test_entry_t *t) {
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
  if (result == KTEST_FAILURE)
    ktest_failure();
  return result;
}

inline static int test_is_autorunnable(test_entry_t *t) {
  return !(t->flags & KTEST_FLAG_NORETURN) && !(t->flags & KTEST_FLAG_DIRTY) &&
         !(t->flags & KTEST_FLAG_BROKEN);
}

static int test_name_compare(const void *a_, const void *b_) {
  const test_entry_t *a = *(test_entry_t **)a_;
  const test_entry_t *b = *(test_entry_t **)b_;
  return strncmp(a->test_name, b->test_name, KTEST_NAME_MAX);
}

static void run_all_tests() {
  kmalloc_init(test_pool);
  kmalloc_add_pages(test_pool, 1);

  /* First, count the number of tests that may be run in any order. */
  unsigned int n = 0;
  SET_DECLARE(tests, test_entry_t);
  test_entry_t **ptr;
  SET_FOREACH(ptr, tests) {
    if (test_is_autorunnable(*ptr))
      n++;
  }
  /* Now, allocate memory for test we'll be running. */
  autorun_tests = kmalloc(test_pool, (n + 1) * sizeof(test_entry_t *), M_ZERO);
  /* Collect test pointers. */
  int i = 0;
  SET_FOREACH(ptr, tests) {
    if (test_is_autorunnable(*ptr))
      autorun_tests[i++] = *ptr;
  }
  autorun_tests[i] = NULL;

  /* Sort tests alphabetically by name, so that shuffling may be deterministic
   * and not affected by build/link order. */
  qsort(autorun_tests, n, sizeof(autorun_tests), test_name_compare);

  /* TODO: Shuffle autorun_tests pointers using seed from command line! */

  kprintf("Found %d automatically runnable tests.\n", n);
  kprintf("Planned test order:\n");
  for (i = 0; i < n; i++)
    kprintf("  %s\n", autorun_tests[i]->test_name);

  for (i = 0; i < n; i++) {
    current_test = autorun_tests[i];
    /* If the test fails, run_test will not return. */
    run_test(current_test);
  }

  /* If we've managed to get here, it means all tests passed with no issues. */
  kprintf(TEST_PASSED_STRING);
}

int main() {
  const char *init = kenv_get("init");
  const char *test = kenv_get("test");

  if (init)
    run_init(init);
  else if (test) {
    if (strncmp(test, "all", 3) == 0) {
      run_all_tests();
    } else {
      /* Single test mode */
      test_entry_t *t = find_test_by_name(test);
      if (!t) {
        kprintf("Test \"%s\" not found!", test);
        return 1;
      }
      int result = run_test(t);
      if (result == KTEST_SUCCESS)
        kprintf(TEST_PASSED_STRING);
    }
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
