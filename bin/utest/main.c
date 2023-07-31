#include "utest.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

SET_DECLARE(tests, test_entry_t);

static int utest_repeat = 1;
static int utest_seed = 0;

typedef struct job {
  test_entry_t *te;
  pid_t pid;
  int status;
} job_t;

static job_t *jobs = NULL;
static int njobmax = 1;
static sigset_t sigchld_mask;

static void sigchld_handler(__unused int sig) {
  int old_errno = errno;

  pid_t pid;
  int status;

  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    int found = 0;

    for (int j = 0; j < njobmax; j++) {
      job_t *job = &jobs[j];

      if (job->pid == pid) {
        job->status = status;
        found = 1;
        break;
      }
    }

    if (!found)
      fprintf(stderr, "utest: reaped somebody's else child (pid=%d)!\n", pid);
  }

  errno = old_errno;
}

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

static int running(void) {
  int pending = 0;

  for (int i = 0; i < njobmax; i++) {
    job_t *job = &jobs[i];
    if (job->te == NULL)
      continue;
    if (job->status < 0) {
      pending++;
      continue;
    }
    if (WIFEXITED(job->status)) {
      int code = WEXITSTATUS(job->status);
      if (code) {
        fprintf(stderr, "Test '%s' exited with %d code!\n", job->te->name,
                code);
        exit(EXIT_FAILURE);
      }
    } else if (WIFSIGNALED(job->status)) {
      fprintf(stderr, "Test '%s' was terminated by %s!\n", job->te->name,
              sys_signame[WTERMSIG(job->status)]);
    } else {
      fprintf(stderr, "Test '%s' exited with invalid status %d!\n",
              job->te->name, job->status);
      exit(EXIT_FAILURE);
    }
    job->te = NULL;
    job->status = 0;
    job->pid = 0;
  }

  return pending;
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

  for (int i = 0; i < njobmax; i++) {
    job_t *job = &jobs[i];
    if (job->te == NULL) {
      job->te = te;
      job->pid = pid;
      job->status = -1;
      return;
    }
  }

  abort();
}

static inline int test_disabled(test_entry_t *te) {
  return te->flags < 0;
}

static int test_compare(const void *a_, const void *b_) {
  const test_entry_t *a = *(test_entry_t **)a_;
  const test_entry_t *b = *(test_entry_t **)b_;
  return strcmp(a->name, b->name);
}

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

  const char *parallel_str = getenv("parallel");
  if (parallel_str)
    njobmax = strtoul(parallel_str, NULL, 10);

  fprintf(stderr, "utest: pid=%u seed=%u repeat=%u parallel=%d test=%s\n",
          getpid(), utest_seed, utest_repeat, njobmax, argv[1]);

  select_tests(argv[1]);

  int ntests;
  test_entry_t **tests = collect_tests(&ntests);

  if (utest_seed != 0) {
    /* Initialize LCG with seed.*/
    unsigned seed = utest_seed;
    /* Yates-Fisher shuffle. */
    for (int i = 0; i <= ntests - 2; i++) {
      int j = i + rand_r(&seed) % (ntests - i);
      test_entry_t *swap = tests[i];
      tests[i] = tests[j];
      tests[j] = swap;
    }
  }

  jobs = calloc(sizeof(job_t), njobmax);

  sigset_t mask;

  xsignal(SIGCHLD, sigchld_handler);
  sigemptyset(&sigchld_mask);
  sigaddset(&sigchld_mask, SIGINT);
  xsigprocmask(SIG_BLOCK, &sigchld_mask, &mask);

  for (int i = 0; i < ntests; i++) {
    run_test(tests[i]);

    while (running() == njobmax)
      sigsuspend(&mask);
  }

  while (running() > 0)
    sigsuspend(&mask);

  xsigprocmask(SIG_SETMASK, &mask, NULL);

  return 0;
}
