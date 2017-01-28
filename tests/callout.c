#include <stdc.h>
#include <callout.h>
#include <ktest.h>
#include <mutex.h>
#include <condvar.h>

static mtx_t foo_mtx;
static condvar_t foo_cv;

/* This test verifies whether callouts work at all. */
static void callout_simple(void *arg) {
  kprintf("The callout got executed!\n");

  mtx_lock(&foo_mtx);
  cv_signal(&foo_cv);
  mtx_unlock(&foo_mtx);
}

static int test_callout_simple() {
  mtx_init(&foo_mtx, MTX_DEF);
  cv_init(&foo_cv, "test_callout_simple CV");

  callout_t callout;
  callout_setup_relative(&callout, 10, callout_simple, NULL);

  mtx_lock(&foo_mtx);
  cv_wait(&foo_cv, &foo_mtx);
  mtx_unlock(&foo_mtx);

  return KTEST_SUCCESS;
}

/* This test checks if the order of execution for scheduled callouts is correct.
 */
static int current = 0;

static void callout_ordered(void *arg) {
  int n = (int)arg;
  ktest_assert(current == n);
  current++;

  if (current == 10) {
    mtx_lock(&foo_mtx);
    cv_signal(&foo_cv);
    mtx_unlock(&foo_mtx);
  }
}

static int test_callout_order() {
  mtx_init(&foo_mtx, MTX_DEF);
  cv_init(&foo_cv, "test_callout_order CV");

  current = 0;
  int order[10] = {2, 5, 4, 6, 9, 0, 8, 1, 3, 7};
  callout_t callouts[10];
  for (int i = 0; i < 10; i++)
    callout_setup_relative(&callouts[i], 10 + order[i] * 4, callout_ordered,
                           (void *)order[i]);

  mtx_lock(&foo_mtx);
  cv_wait(&foo_cv, &foo_mtx);
  mtx_unlock(&foo_mtx);
  ktest_assert(current == 10);

  return KTEST_SUCCESS;
}

/* This test verifies that callouts removed with callout_stop are not run. */
static void callout_bad(void *arg) {
  ktest_assert(0);
}
static void callout_good(void *arg) {
  mtx_lock(&foo_mtx);
  cv_signal(&foo_cv);
  mtx_unlock(&foo_mtx);
}

static int test_callout_stop() {
  mtx_init(&foo_mtx, MTX_DEF);
  cv_init(&foo_cv, "test_callout_order CV");

  current = 0;
  callout_t callout1, callout2;
  callout_setup_relative(&callout1, 10, callout_bad, NULL);
  callout_setup_relative(&callout2, 20, callout_good, NULL);

  /* Remove callout1, hope that callout_bad won't be called! */
  callout_stop(&callout1);

  /* Give some time for callout_bad, wait for callout_good. */
  mtx_lock(&foo_mtx);
  cv_wait(&foo_cv, &foo_mtx);
  mtx_unlock(&foo_mtx);

  return KTEST_SUCCESS;
}

KTEST_ADD(callout_simple, test_callout_simple, 0);
KTEST_ADD(callout_order, test_callout_order, 0);
KTEST_ADD(callout_stop, test_callout_stop, 0);
