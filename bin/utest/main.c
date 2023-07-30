#include "utest.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

SET_DECLARE(tests, test_entry_t);

static test_entry_t *find_test(const char *name) {
  test_entry_t **ptr;
  SET_FOREACH (ptr, tests) {
    if (strcmp((*ptr)->name, name) == 0) {
      return *ptr;
    }
  }
  return NULL;
}

static timeval_t timestamp(void) {
  timespec_t ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (timeval_t){.tv_sec = ts.tv_sec, .tv_usec = ts.tv_nsec / 1000};
}

static void run_test(test_entry_t *te) {
  timeval_t tv = timestamp();
  const char *name = te->name;

  fprintf(stderr, "[%d.%06d] Begin '%s' test.\n", (int)tv.tv_sec, tv.tv_usec,
          name);

  pid_t pid = xfork();
  if (pid == 0) {
    setsid();
    setpgid(0, 0);

    if (te->flags & TF_DEBUG)
      __verbose = 1;

    exit(te->func());
  }

  int status;
  waitpid(pid, &status, 0);

  if (WIFEXITED(status)) {
    int code = WEXITSTATUS(status);
    if (code) {
      fprintf(stderr, "Test '%s' exited with %d code!\n", name, code);
      exit(EXIT_FAILURE);
    }
  } else if (WIFSIGNALED(status)) {
    fprintf(stderr, "Test '%s' was terminated by %s!\n", name,
            sys_signame[WTERMSIG(status)]);
  } else {
    fprintf(stderr, "Test '%s' exited with invalid status %d!\n", name, status);
    exit(EXIT_FAILURE);
  }
}

static inline int test_disabled(test_entry_t *te) {
  return te->flags < 0;
}

static int test_compare(const void *a_, const void *b_) {
  const test_entry_t *a = *(test_entry_t **)a_;
  const test_entry_t *b = *(test_entry_t **)b_;
  return strcmp(a->name, b->name);
}

static int utest_repeat = 1;
static int utest_seed = 0;
static unsigned seed = 0;

static void select_tests(char *test_str) {
  if (!strcmp(test_str, "all"))
    return;

  /* Disable all tests and later enable them one by one. */
  test_entry_t **ptr;
  SET_FOREACH (ptr, tests) {
    (*ptr)->flags |= TF_DISABLED;
  }

  char *brkt;
  for (char *name = strtok_r(test_str, ",", &brkt); name;
       name = strtok_r(NULL, ",", &brkt)) {
    test_entry_t *te = find_test(name);
    if (!te) {
      fprintf(stderr, "Test '%s' not found!", name);
      exit(EXIT_FAILURE);
    }
    te->flags &= ~TF_DISABLED;
  }
}

/* Count the number of tests that may be run in an arbitrary order. */
static test_entry_t **collect_tests(int *countp) {
  int n = 2;
  int i = 0;

  test_entry_t **vector = malloc(sizeof(test_entry_t *) * n);

  test_entry_t **ptr;
  SET_FOREACH (ptr, tests) {
    test_entry_t *te = *ptr;
    if (test_disabled(te)) {
      fprintf(stderr, "Test '%s' skipped.\n", te->name);
      continue;
    }

    for (int r = 0; r < utest_repeat; r++) {
      if (i == n - 1) {
        n *= 2;
        vector = realloc(vector, sizeof(test_entry_t *) * n);
      }
      vector[i++] = *ptr;
    }
  }

  vector[i] = NULL;
  *countp = i;

  /* Sort tests alphabetically by name, so that shuffling may be deterministic
   * and not affected by build/link order. */
  qsort(vector, i, sizeof(test_entry_t *), test_compare);
  return vector;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Not enough arguments provided to utest.\n");
    return 1;
  }

  const char *seed_str = getenv("seed");
  if (seed_str)
    utest_seed = strtoul(seed_str, NULL, 10);

  const char *repeat_str = getenv("repeat");
  if (repeat_str)
    utest_repeat = strtoul(repeat_str, NULL, 10);

  fprintf(stderr, "utest: seed=%u repeat=%u test=%s\n", utest_seed,
          utest_repeat, argv[1]);

  select_tests(argv[1]);

  int ntests;
  test_entry_t **tests = collect_tests(&ntests);

  if (utest_seed != 0) {
    /* Initialize LCG with seed.*/
    seed = utest_seed;
    /* Yates-Fisher shuffle. */
    for (int i = 0; i <= ntests - 2; i++) {
      int j = i + rand_r(&seed) % (ntests - i);
      test_entry_t *swap = tests[i];
      tests[i] = tests[j];
      tests[j] = swap;
    }
  }

  for (int i = 0; i < ntests; i++)
    run_test(tests[i]);

  return 0;
}
