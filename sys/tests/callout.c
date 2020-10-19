#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/callout.h>
#include <sys/ktest.h>
#include <sys/interrupt.h>

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
  int ord = (intptr_t)arg;
  assert(current == ord);
  /* There is no race condition here since callouts don't run concurrently. */
  current++;
}

static int test_callout_order(void) {
  callout_t callouts[ORDER_N];
  bzero(callouts, sizeof(callout_t) * ORDER_N);
  current = 0;

  systime_t now = getsystime();
  for (int i = 0; i < ORDER_N; i++)
    callout_setup(&callouts[i], now + 5 + order[i] * 5, callout_ordered,
                  (void *)(intptr_t)order[i]);

  /* Wait for all callouts. */
  for (int i = 0; i < ORDER_N; i++)
    callout_drain(&callouts[i]);

  return KTEST_SUCCESS;
}

/* This test verifies that callouts removed with callout_stop are not run. */
static void callout_bad(void *arg) {
  panic("%s: should never be called!", __func__);
}

static callout_t callout;

static int test_callout_stop(void) {
  bzero(&callout, sizeof(callout_t));

  /* XXX This is a temporary solution to make sure that the callout
   * isn't migrated to the `delegated` queue.
   * Ideally, disabling preemption should be enough. */
  WITH_INTR_DISABLED {
    callout_setup_relative(&callout, 2, callout_bad, NULL);
    /* Remove callout, hope that callout_bad won't be called! */
    callout_stop(&callout);
    /* We don't drain this callout so its memory can still be in use after we
     * leave the scope of function. Thus the callout is allocated statically. */
  }

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

KTEST_ADD(callout_simple, test_callout_simple, 0);
KTEST_ADD(callout_order, test_callout_order, 0);
KTEST_ADD(callout_stop, test_callout_stop, 0);
KTEST_ADD(callout_drain, test_callout_drain, 0);
