#include <stdc.h>
#include <callout.h>
#include <ktest.h>
#include <interrupt.h>

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

static int counter;

/* This test verifies whether callouts work at all. */
static void callout_increment(void *arg) {
  counter++;
}

static int test_callout_simple(void) {
  const int N = 100;

  callout_t callout;
  bzero(&callout, sizeof(callout_t));

  counter = 0;

  for (int i = 0; i < N; i++) {
    callout_setup_relative(&callout, 1, callout_increment, NULL);
    callout_drain(&callout);
  }

  assert(counter == N);

  return KTEST_SUCCESS;
}

/* This test checks if the order of execution for scheduled callouts is correct.
 */

#define ORDER_N 10
static int order[ORDER_N] = {2, 5, 4, 6, 9, 0, 8, 1, 3, 7};
static int current;

static void callout_ordered(void *arg) {
  /* There is no race condition here as callouts run in bottom half (with
   * interrupts disabled). */
  assert(intr_disabled());

  int ord = (int)arg;
  assert(current == ord);
  current++;
}

static int test_callout_order(void) {
  callout_t callouts[ORDER_N];
  bzero(callouts, sizeof(callout_t) * ORDER_N);
  current = 0;

  systime_t now = getsystime();
  for (int i = 0; i < ORDER_N; i++)
    callout_setup(&callouts[i], now + 5 + order[i] * 5, callout_ordered,
                  (void *)order[i]);

  /* Wait for all callouts. */
  for (int i = 0; i < ORDER_N; i++)
    callout_drain(&callouts[i]);

  return KTEST_SUCCESS;
}

/* This test verifies that callouts removed with callout_stop are not run. */
static void callout_bad(void *arg) {
  panic("%s: should never be called!", __func__);
}

static int test_callout_stop(void) {
  callout_t callout;
  bzero(&callout, sizeof(callout_t));

  callout_setup_relative(&callout, 2, callout_bad, NULL);
  /* Remove callout, hope that callout_bad won't be called! */
  callout_stop(&callout);

  return KTEST_SUCCESS;
}

static void callout_to_stop(void *arg) {
  /* Nothing important. */
}

static int test_callout_drain(void) {
  callout_t callout;
  bzero(&callout, sizeof(callout_t));

  callout_setup_relative(&callout, 10, callout_to_stop, NULL);
  callout_stop(&callout);
  bool drained = callout_drain(&callout);

  /* There was no need to drain as callout has been already stopped. */
  assert(!drained);

  return KTEST_SUCCESS;
}

KTEST_ADD(callout_sync, test_callout_sync, 0);
KTEST_ADD(callout_simple, test_callout_simple, 0);
KTEST_ADD(callout_order, test_callout_order, 0);
KTEST_ADD(callout_stop, test_callout_stop, 0);
KTEST_ADD(callout_drain, test_callout_drain, 0);
