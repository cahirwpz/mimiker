#include <thread.h>
#include <ktest.h>
#include <sched.h>
#include <runq.h>

#define THREADS_NUMBER 10
#define TEST_TIME 1000

static void thread_function(void *arg) {
  while (clock_get() < TEST_TIME + *(realtime_t *)arg)
    ;
}

static int test_thread_stats(void) {
  thread_t *threads[THREADS_NUMBER];
  realtime_t start = clock_get();
  for (int i = 0; i < THREADS_NUMBER; i++) {
    threads[i] =
      thread_create("threads time stats test", thread_function, &start);
    sched_add(threads[i]);
  }
  for (int i = 0; i < THREADS_NUMBER; i++) {
    thread_join(threads[i]);
  }
  for (int i = 0; i < THREADS_NUMBER; i++) {
    log("Thread:%d runtime:%lu sleeptime:%lu context switches:%llu", i,
        threads[i]->td_rtime, threads[i]->td_slptime, threads[i]->td_nctxsw);
  }
  return KTEST_SUCCESS;
}

KTEST_ADD(thread_stats, test_thread_stats, 0);
