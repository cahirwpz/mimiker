#include <stdc.h>
#include <callout.h>
#include <ktest.h>
#include <sleepq.h>
#include <interrupt.h>

static int counter;

static void periodic_callout(void *arg) {
  callout_t *callout = arg;
  callout_setup_relative(callout, 1, periodic_callout, arg);
}

static int test_callout_sync(void) {
  const int K = 50;
  const int N = 7;

  callout_t callout[N];
  bzero(callout, sizeof(callout_t) * N);

  for (int j = 0; j < K; j++) {
    for (int i = 0; i < N; i++)
      callout_setup_relative(&callout[i], 1, periodic_callout, &callout[i]);

    for (int i = 0; i < N; i++)
      callout_stop(&callout[i]);
  }

  return KTEST_SUCCESS;
}

/* This test verifies whether callouts work at all. */
static void callout_simple(void *arg) {
  counter++;
  sleepq_signal(callout_simple);
}

static int test_callout_simple(void) {
  const int N = 100;

  callout_t callout;
  bzero(&callout, sizeof(callout_t));

  counter = 0;

  for (int i = 0; i < N; i++) {
    WITH_INTR_DISABLED {
      callout_setup_relative(&callout, 1, callout_simple, NULL);
      sleepq_wait(callout_simple, "callout_simple");
    }
  }

  assert(counter == N);

  return KTEST_SUCCESS;
}

/* This test checks if the order of execution for scheduled callouts is correct.
 */
static int current = 0;

static void callout_ordered(void *arg) {
  assert(intr_disabled());
  /* This incrementation is safe as callouts run with interrupts disabled. */
  current++;

  if (current == 10)
    sleepq_signal(callout_ordered);
}

static int test_callout_order(void) {
  current = 0;

  int order[10] = {2, 5, 4, 6, 9, 0, 8, 1, 3, 7};
  callout_t callouts[10];
  bzero(callouts, sizeof(callout_t) * 10);

  /* Register callouts within a critical section, to ensure they use the same
     base time! */
  WITH_INTR_DISABLED {
    for (int i = 0; i < 10; i++)
      callout_setup_relative(&callouts[i], 5 + order[i] * 5, callout_ordered,
                             (void *)order[i]);

    sleepq_wait(callout_ordered, "callout_ordered");
  }

  assert(current == 10);

  return KTEST_SUCCESS;
}

/* This test verifies that callouts removed with callout_stop are not run. */
static void callout_bad(void *arg) {
  assert(0);
}

static void callout_good(void *arg) {
  sleepq_signal(callout_good);
}

static int test_callout_stop(void) {
  current = 0;
  callout_t callout1, callout2;
  bzero(&callout1, sizeof(callout_t));
  bzero(&callout2, sizeof(callout_t));

  SCOPED_INTR_DISABLED();

  callout_setup_relative(&callout1, 5, callout_bad, NULL);
  callout_setup_relative(&callout2, 10, callout_good, NULL);

  /* Remove callout1, hope that callout_bad won't be called! */
  callout_stop(&callout1);

  /* Give some time for callout_bad, wait for callout_good. */
  sleepq_wait(callout_good, "callout_good");

  return KTEST_SUCCESS;
}

KTEST_ADD(callout_sync, test_callout_sync, 0);
KTEST_ADD(callout_simple, test_callout_simple, 0);
KTEST_ADD(callout_order, test_callout_order, 0);
KTEST_ADD(callout_stop, test_callout_stop, 0);
