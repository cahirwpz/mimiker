#include <thread.h>
#include <ktest.h>
#include <klog.h>
#include <sched.h>
#include <runq.h>

#define THREADS_NUMBER 10

static timeval_t test_time = TIMEVAL(0.2);

static void thread_nop_function(void *arg) {
  timeval_t end = timeval_add(arg, &test_time);
  timeval_t now = get_uptime();
  while (timeval_cmp(&now, &end, <))
    now = get_uptime();
}

static int test_thread_stats_nop(void) {
  thread_t *threads[THREADS_NUMBER];
  timeval_t start = get_uptime();
  for (int i = 0; i < THREADS_NUMBER; i++) {
    threads[i] = thread_create("nop thread", thread_nop_function, &start);
    sched_add(threads[i]);
  }
  for (int i = 0; i < THREADS_NUMBER; i++) {
    thread_join(threads[i]);
  }
  for (int i = 0; i < THREADS_NUMBER; i++) {
    thread_t *td = threads[i];
    klog("Thread:%d runtime:%u.%u sleeptime:%u.%u context switches:%llu", i,
         td->td_rtime.tv_sec, td->td_rtime.tv_usec, td->td_slptime.tv_sec,
         td->td_slptime.tv_usec, td->td_nctxsw);
    if (!timeval_isset(&td->td_rtime) && timeval_isset(&td->td_slptime))
      return KTEST_FAILURE;
  }

  return KTEST_SUCCESS;
}

static void thread_wake_function(void *arg) {
  timeval_t end = timeval_add(arg, &test_time);
  end = timeval_add(&end, &TIMEVAL(0.1));
  timeval_t now = get_uptime();
  while (timeval_cmp(&now, &end, <)) {
    sleepq_broadcast(arg);
    now = get_uptime();
  }
}

static void thread_sleep_function(void *arg) {
  sleepq_wait(arg, "Thread stats test sleepq");
  timeval_t end = timeval_add(arg, &test_time);
  timeval_t now = get_uptime();
  while (timeval_cmp(&now, &end, <)) {
    sleepq_wait(arg, "Thread stats test sleepq");
    now = get_uptime();
  }
}

static int test_thread_stats_slp(void) {
  thread_t *threads[THREADS_NUMBER];
  timeval_t start = get_uptime();
  for (int i = 0; i < THREADS_NUMBER; i++) {
    threads[i] = thread_create("sleeper thread", thread_sleep_function, &start);
    sched_add(threads[i]);
  }
  thread_t *waker = thread_create("waker thread", thread_wake_function, &start);
  sched_add(waker);
  for (int i = 0; i < THREADS_NUMBER; i++) {
    thread_join(threads[i]);
  }
  thread_join(waker);
  for (int i = 0; i < THREADS_NUMBER; i++) {
    thread_t *td = threads[i];
    klog("Thread: %d, runtime: %u.%u, sleeptime: %u.%u, context switches: %llu",
         i, td->td_rtime.tv_sec, td->td_rtime.tv_usec, td->td_slptime.tv_sec,
         td->td_slptime.tv_usec, td->td_nctxsw);
    if (!timeval_isset(&td->td_rtime) || !timeval_isset(&td->td_slptime))
      return KTEST_FAILURE;
  }
  return KTEST_SUCCESS;
}

/* TODO: These tests take too long to run to be a part of regular test run.
 * For now we have to run them manually. */
KTEST_ADD(thread_stats_nop, test_thread_stats_nop, KTEST_FLAG_BROKEN);
KTEST_ADD(thread_stats_slp, test_thread_stats_slp, KTEST_FLAG_BROKEN);
