#include <stdc.h>
#include <callout.h>
#include <ktest.h>
#include <sleepq.h>
#include <sync.h>

/* This test verifies whether callouts work at all. */
static void callout_simple(void *arg) {
   (void)arg;
  kprintf("The callout got executed!\n");

  sleepq_signal(callout_simple);
}

static int test_callout_simple() {
  callout_t callout;

  critical_enter();
  callout_setup_relative(&callout, 5, callout_simple, NULL);
  sleepq_wait(callout_simple, "callout_simple");
  critical_leave();

  return KTEST_SUCCESS;
}

/* This test checks if the order of execution for scheduled callouts is correct.
 */
static int current = 0;

static void callout_ordered(void *arg) {
   (void)arg;
  /* Atomic increment */
  critical_enter();
  current++;
  critical_leave();

  if (current == 10)
    sleepq_signal(callout_ordered);
}

static int test_callout_order() {
  current = 0;
  int order[10] = {2, 5, 4, 6, 9, 0, 8, 1, 3, 7};
  callout_t callouts[10];

  /* Register callouts within a critical section, to ensure they use the same
     base time! */
  critical_enter();
  for (int i = 0; i < 10; i++)
    callout_setup_relative(&callouts[i], 5 + order[i] * 5, callout_ordered,
                           (void *)order[i]);

  sleepq_wait(callout_ordered, "callout_ordered");
  critical_leave();

  assert(current == 10);

  return KTEST_SUCCESS;
}

/* This test verifies that callouts removed with callout_stop are not run. */
static void callout_bad(void *arg) {
   (void)arg;
  assert(0);
}

static void callout_good(void *arg) {
   (void)arg;
  sleepq_signal(callout_good);
}

static int test_callout_stop() {
  current = 0;
  callout_t callout1, callout2;

  critical_enter();

  callout_setup_relative(&callout1, 5, callout_bad, NULL);
  callout_setup_relative(&callout2, 10, callout_good, NULL);

  /* Remove callout1, hope that callout_bad won't be called! */
  callout_stop(&callout1);

  /* Give some time for callout_bad, wait for callout_good. */
  sleepq_wait(callout_good, "callout_good");

  critical_leave();

  return KTEST_SUCCESS;
}

KTEST_ADD(callout_simple, test_callout_simple, 0);
KTEST_ADD(callout_order, test_callout_order, 0);
KTEST_ADD(callout_stop, test_callout_stop, 0);
