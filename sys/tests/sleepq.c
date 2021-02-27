#include <sys/libkern.h>
#include <sys/callout.h>
#include <sys/ktest.h>
#include <sys/sleepq.h>
#include <sys/thread.h>
#include <sys/sched.h>

static volatile int wakeups;

static void test_thread(void *expected) {
  while (wakeups < (intptr_t)expected) {
    WITH_NO_PREEMPTION {
      wakeups++;
    }
    sleepq_wait(&test_thread, NULL);
  }
}

static void wake_threads_up(void *arg) {
  sleepq_broadcast(&test_thread);
}

static int test_sleepq_sync(void) {
  const int K = 5;  /* number of test threads */
  const int N = 20; /* number of expected wakeups */

  wakeups = 0;

  callout_t callout[N];
  thread_t *td[K];

  for (int i = 0; i < N; i++)
    callout_setup(&callout[i], wake_threads_up, NULL);

  /* Create the threads before setting up the callouts so that not a single
   * wake-up will be lost! */
  for (int i = 0; i < K; i++) {
    td[i] = thread_create("test-sleepq", test_thread, (void *)(intptr_t)N,
                          prio_kthread(0));
    sched_add(td[i]);
  }

  for (int i = 0; i < N; i++)
    callout_schedule(&callout[i], i + 1);

  for (int i = 0; i < K; i++)
    thread_join(td[i]);

  /* After callouts are drained we know that their memory is not used
   * by callout_thread so it's safe to allocate them on stack. */
  for (int i = 0; i < N; i++)
    callout_drain(&callout[i]);

  return KTEST_SUCCESS;
}

KTEST_ADD(sleepq_sync, test_sleepq_sync, 0);
