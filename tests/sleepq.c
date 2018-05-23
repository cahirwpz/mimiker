#include <stdc.h>
#include <callout.h>
#include <ktest.h>
#include <sleepq.h>
#include <thread.h>
#include <sched.h>

static volatile int wakeups;

static void test_thread(void *p) {
  while (wakeups < (int)p) {
    WITH_NO_PREEMPTION {
      wakeups++;
    }
    sleepq_wait(&test_thread, NULL);
  }
}

static void periodic_callout(void *arg) {
  callout_t *callout = arg;
  callout_setup_relative(callout, 1, periodic_callout, callout);
  sleepq_broadcast(&test_thread);
}

static int test_sleepq_sync(void) {
  const int K = 5;
  const int N = 20;

  wakeups = 0;

  callout_t callout;
  thread_t *td[K];

  callout_setup_relative(&callout, 1, periodic_callout, &callout);

  for (int i = 0; i < K; i++) {
    td[i] = thread_create("test-thread", test_thread, (void *)N);
    sched_add(td[i]);
  }

  for (int i = 0; i < K; i++)
    thread_join(td[i]);

  callout_stop(&callout);

  return KTEST_SUCCESS;
}

KTEST_ADD(sleepq_sync, test_sleepq_sync, 0);
