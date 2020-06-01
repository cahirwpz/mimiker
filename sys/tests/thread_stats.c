#include <sys/thread.h>
#include <sys/ktest.h>
#include <sys/klog.h>
#include <sys/sched.h>
#include <sys/sleepq.h>
#include <sys/runq.h>

#define THREADS_NUMBER 10

/* 0.2 as a bintime fraction */
static uint64_t test_time_frac = 3689348814741910528;

static void thread_nop_function(void *arg) {
  bintime_t end = *(bintime_t *)arg;
  bintime_add_frac(&end, test_time_frac);
  bintime_t now = getbintime();
  while (bintime_cmp(&now, &end, <))
    now = getbintime();
}

static int test_thread_stats_nop(void) {
  thread_t *threads[THREADS_NUMBER];
  bintime_t start = getbintime();
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
    klog(
      "Thread:%d runtime:%llu.%llu sleeptime:%llu.%llu context switches:%llu",
      i, td->td_rtime.sec, td->td_rtime.frac, td->td_slptime.sec,
      td->td_slptime.frac, td->td_nctxsw);
    if (!bintimeisset(&td->td_rtime) && bintimeisset(&td->td_slptime))
      return KTEST_FAILURE;
  }

  return KTEST_SUCCESS;
}

static void thread_wake_function(void *arg) {
  bintime_t end = *(bintime_t *)arg;
  bintime_add_frac(&end, test_time_frac);
  bintime_add(&end, &BINTIME(0.1));
  bintime_t now = getbintime();
  while (bintime_cmp(&now, &end, <)) {
    sleepq_broadcast(arg);
    now = getbintime();
  }
}

static void thread_sleep_function(void *arg) {
  sleepq_wait(arg, "Thread stats test sleepq");
  bintime_t end = *(bintime_t *)arg;
  bintime_add_frac(&end, test_time_frac);
  bintime_t now = getbintime();
  while (bintime_cmp(&now, &end, <)) {
    sleepq_wait(arg, "Thread stats test sleepq");
    now = getbintime();
  }
}

static int test_thread_stats_slp(void) {
  thread_t *threads[THREADS_NUMBER];
  bintime_t start = getbintime();
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
    klog("Thread: %d, runtime: %llu.%llu, sleeptime: %llu.%llu, context "
         "switches: %llu",
         i, td->td_rtime.sec, td->td_rtime.frac, td->td_slptime.sec,
         td->td_slptime.frac, td->td_nctxsw);
    if (!bintimeisset(&td->td_rtime) || !bintimeisset(&td->td_slptime))
      return KTEST_FAILURE;
  }
  return KTEST_SUCCESS;
}

/* TODO: These tests take too long to run to be a part of regular test run.
 * For now we have to run them manually. */
KTEST_ADD(thread_stats_nop, test_thread_stats_nop, KTEST_FLAG_BROKEN);
KTEST_ADD(thread_stats_slp, test_thread_stats_slp, KTEST_FLAG_BROKEN);
