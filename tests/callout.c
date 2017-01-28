#include <stdc.h>
#include <callout.h>
#include <ktest.h>
#include <mutex.h>
#include <condvar.h>

static mtx_t foo_mtx;
static condvar_t foo_cv;

static void callout_foo(void *arg) {
  kprintf("The callout got executed!\n");

  mtx_lock(&foo_mtx);
  cv_signal(&foo_cv);
  mtx_unlock(&foo_mtx);
}

static int test_callout_simple() {
  mtx_init(&foo_mtx, MTX_DEF);
  cv_init(&foo_cv, "test_callout_simple CV");

  callout_t callout;
  callout_setup_relative(&callout, 10, callout_foo, NULL);

  mtx_lock(&foo_mtx);
  cv_wait(&foo_cv, &foo_mtx);
  mtx_unlock(&foo_mtx);
  kprintf("OK\n");

  return KTEST_SUCCESS;
}

KTEST_ADD(callout_simple, test_callout_simple, 0);
