#include <ktest.h>
#include <stdc.h>
#include <sync.h>
#include <thread.h>
#include <callout.h>

/* Borrowed from mips/malta.c */
char *kenv_get(const char *key);

#define KTEST_MAX_NO 256

/* Stores currently running test data. */
static test_entry_t *current_test = NULL;
/* A null-terminated array of pointers to the tested test list. */
static test_entry_t *autorun_tests[KTEST_MAX_NO] = {NULL};

int ktest_test_running_flag = 0;

static uint32_t init_seed = 0; /* The initial seed, as set from command-line. */
static uint32_t seed = 0;      /* Current seed */
static uint32_t rand() {
  /* Just a standard LCG */
  seed = 1664525 * seed + 1013904223;
  return seed;
}

/* If we get preempted while printing out the [TEST_PASSED] string, the monitor
   process might not find the pattern it's looking for. */
static void ktest_atomically_print_success() {
  critical_enter();
  kprintf(TEST_PASSED_STRING);
  critical_leave();
}
static void ktest_atomically_print_failure() {
  critical_enter();
  kprintf(TEST_FAILED_STRING);
  critical_leave();
}

void ktest_failure() {
  if (current_test == NULL)
    panic("current_test == NULL in ktest_failure! This is most likely a bug in "
          "the test framework!\n");
  ktest_atomically_print_failure();
  if (autorun_tests[0]) {
    kprintf("Failure while running multiple tests.\n");
    for (test_entry_t **ptr = autorun_tests; *ptr != NULL; ptr++) {
      test_entry_t *t = *ptr;
      kprintf("  %s", t->test_name);
      if (t == current_test) {
        kprintf("  <---- FAILED\n");
        break;
      } else {
        kprintf("\n");
      }
    }
    kprintf("The seed used for this test order was: %ld. Start kernel with "
            "`test=all seed=%ld` to reproduce this test case.\n",
            init_seed, init_seed);
  } else {
    kprintf("Failure while running single test.\n");
    kprintf("Failing test: %s\n", current_test->test_name);
  }
  panic("Halting kernel on failed test.\n");
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
  ktest_test_running_flag = 1;
  int result = t->test_func();
  ktest_test_running_flag = 0;
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
  /* First, count the number of tests that may be run in any order. */
  unsigned int n = 0;
  SET_DECLARE(tests, test_entry_t);
  test_entry_t **ptr;
  SET_FOREACH(ptr, tests) {
    if (test_is_autorunnable(*ptr))
      n++;
  }
  if (n > KTEST_MAX_NO + 1) {
    kprintf("Warning: There are more kernel tests registered than there is "
            "memory available for ktest framework. Please increase "
            "KTEST_MAX_NO.\n");
    ktest_atomically_print_failure();
    return;
  }

  /* Collect test pointers. */
  int i = 0;
  SET_FOREACH(ptr, tests) {
    if (test_is_autorunnable(*ptr))
      autorun_tests[i++] = *ptr;
  }
  autorun_tests[i] = NULL;

  /* Sort tests alphabetically by name, so that shuffling may be deterministic
   * and not affected by build/link order. */
  qsort(autorun_tests, n, sizeof(test_entry_t *), test_name_compare);

  const char *seed_str = kenv_get("seed");
  if (seed_str)
    init_seed = strtoul(seed_str, NULL, 10);

  if (init_seed != 0) {
    /* Initialize LCG with seed.*/
    seed = init_seed;
    /* Yates-Fisher shuffle. */
    for (i = 0; i <= n - 2; i++) {
      int j = i + rand() % (n - i);
      register test_entry_t *swap = autorun_tests[i];
      autorun_tests[i] = autorun_tests[j];
      autorun_tests[j] = swap;
    }
  }

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
  ktest_atomically_print_success();

  /* As the tests are usually very verbose, for user convenience let's print out
     the order of tests once again. */
  kprintf("Test order:\n");
  for (i = 0; i < n; i++)
    kprintf("  %s\n", autorun_tests[i]->test_name);
}

void ktest_main(const char *test) {
  if (strncmp(test, "all", 3) == 0) {
    run_all_tests();
  } else {
    /* Single test mode */
    test_entry_t *t = find_test_by_name(test);
    if (!t) {
      kprintf("Test \"%s\" not found!\n", test);
      return;
    }
    int result = run_test(t);
    if (result == KTEST_SUCCESS)
      ktest_atomically_print_success();
  }
}

void ktest_usermode_exit(thread_t *td, int status) {
  assert(ktest_test_running_flag);

  sleepq_signal(td);
  td->td_ktest_status = status;
}

void ktest_usermode_timeout(void *arg) {
  kprintf("Usermode timed out!\n");
  ktest_failure();
}

void ktest_wait_for_user_thread(thread_t *td, int timeout_ms) {
  callout_t timeout_callout;
  if (timeout_ms > 0)
    callout_setup_relative(&timeout_callout, timeout_ms, ktest_usermode_timeout,
                           NULL);

  sleepq_wait(td, "usermode ktest");
  int status = td->td_ktest_status;

  if (timeout_ms > 0)
    callout_stop(&timeout_callout);

  if (status != 0) {
    kprintf("Usermode thread returned non-zero exit code (%d)!\n", status);
    ktest_failure();
  }
}
