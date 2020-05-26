#include <sys/thread.h>
#include <sys/ktest.h>
#include <sys/klog.h>
#include <sys/sched.h>
#include <sys/sleepq.h>
#include <sys/runq.h>

#define THREADS_NUMBER 10

static timespec_t test_time = TIMESPEC(0.2);

static void thread_nop_function(void *arg) {
  timespec_t end;
  timespecadd((timespec_t *) arg, &test_time, &end);
  timespec_t now = nanouptime();
  while (timespeccmp(&now, &end, <))
    now = nanouptime();
}

static int test_thread_stats_nop(void) {
  thread_t *threads[THREADS_NUMBER];
  timespec_t start = nanouptime();
  for (int i = 0; i < THREADS_NUMBER; i++) {
    threads[i] = thread_create("test-thread-stats-nop", thread_nop_function,
                               &start, prio_kthread(0));
    sched_add(threads[i]);
  }
  for (int i = 0; i < THREADS_NUMBER; i++) {
    thread_join(threads[i]);
  }
  for (int i = 0; i < THREADS_NUMBER; i++) {
    thread_t *td = threads[i];
    klog("Thread:%d runtime:%u.%u sleeptime:%u.%u context switches:%llu", i,
         td->td_rtime.tv_sec, td->td_rtime.tv_nsec, td->td_slptime.tv_sec,
         td->td_slptime.tv_nsec, td->td_nctxsw);
    if (!timespec_isset(&td->td_rtime) && timespec_isset(&td->td_slptime))
      return KTEST_FAILURE;
  }

  return KTEST_SUCCESS;
}

static void thread_wake_function(void *arg) {
  timespec_t end;
  timespecadd((timespec_t *)arg, &test_time, &end);
  timespecadd(&end, &TIMESPEC(0.1), &end);
  timespec_t now = nanouptime();
  while (timespeccmp(&now, &end, <)) {
    sleepq_broadcast(arg);
    now = nanouptime();
  }
}

static void thread_sleep_function(void *arg) {
  sleepq_wait(arg, "Thread stats test sleepq");
  timespec_t end;
  timespecadd((timespec_t *)arg, &test_time, &end);
  timespec_t now = nanouptime();
  while (timespeccmp(&now, &end, <)) {
    sleepq_wait(arg, "Thread stats test sleepq");
    now = nanouptime();
  }
}

static int test_thread_stats_slp(void) {
  thread_t *threads[THREADS_NUMBER];
  timespec_t start = nanouptime();
  for (int i = 0; i < THREADS_NUMBER; i++) {
    threads[i] = thread_create("test-thread-stats-sleeper",
                               thread_sleep_function, &start, prio_kthread(0));
    sched_add(threads[i]);
  }
  thread_t *waker = thread_create(
    "test-thread-stats-waker", thread_wake_function, &start, prio_kthread(0));
  sched_add(waker);
  for (int i = 0; i < THREADS_NUMBER; i++) {
    thread_join(threads[i]);
  }
  thread_join(waker);
  for (int i = 0; i < THREADS_NUMBER; i++) {
    thread_t *td = threads[i];
    klog("Thread: %d, runtime: %u.%u, sleeptime: %u.%u, context switches: %llu",
         i, td->td_rtime.tv_sec, td->td_rtime.tv_nsec, td->td_slptime.tv_sec,
         td->td_slptime.tv_nsec, td->td_nctxsw);
    if (!timespec_isset(&td->td_rtime) || !timespec_isset(&td->td_slptime))
      return KTEST_FAILURE;
  }
  return KTEST_SUCCESS;
}

/* TODO: These tests take too long to run to be a part of regular test run.
 * For now we have to run them manually. */
KTEST_ADD(thread_stats_nop, test_thread_stats_nop, KTEST_FLAG_BROKEN);
KTEST_ADD(thread_stats_slp, test_thread_stats_slp, KTEST_FLAG_BROKEN);
