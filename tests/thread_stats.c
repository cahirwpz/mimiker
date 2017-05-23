#include <thread.h>
#include <ktest.h>
#include <klog.h>
#include <sched.h>
#include <runq.h>

#define THREADS_NUMBER 10
#define TEST_TIME 500

static void thread_nop_function(void *arg) {
  while (tv2st(get_uptime()) < TEST_TIME + tv2st(*(timeval_t *)arg))
    ;
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
    klog("Thread:%d runtime:%u.%u sleeptime:%u.%u context switches:%llu", i,
         threads[i]->td_rtime.tv_sec, threads[i]->td_rtime.tv_usec,
         threads[i]->td_slptime.tv_sec, threads[i]->td_slptime.tv_usec,
         threads[i]->td_nctxsw);
    if ((threads[i]->td_rtime.tv_sec == 0 &&
         threads[i]->td_rtime.tv_usec == 0) ||
        (threads[i]->td_slptime.tv_sec != 0 &&
         threads[i]->td_slptime.tv_usec != 0))
      return KTEST_FAILURE;
  }

  return KTEST_SUCCESS;
}

static void thread_wake_function(void *arg) {
  while (tv2st(get_uptime()) < 2 * TEST_TIME + tv2st(*(timeval_t *)arg))
    if (tv2st(get_uptime()) > tv2st(thread_self()->td_last_rtime) + 10)
      sleepq_broadcast(arg);
}

static void thread_sleep_function(void *arg) {
  while (tv2st(get_uptime()) < TEST_TIME + tv2st(*(timeval_t *)arg))
    if (tv2st(get_uptime()) > tv2st(thread_self()->td_last_rtime))
      sleepq_wait(arg, "Thread stats test sleepq");
}

static int test_thread_stats_slp(void) {
  thread_t *threads[THREADS_NUMBER];
  timeval_t start = get_uptime();
  thread_t *waker = thread_create("waker thread", thread_wake_function, &start);
  sched_add(waker);
  for (int i = 0; i < THREADS_NUMBER; i++) {
    threads[i] = thread_create("sleeper thread", thread_sleep_function, &start);
    sched_add(threads[i]);
  }
  for (int i = 0; i < THREADS_NUMBER; i++) {
    thread_join(threads[i]);
  }
  thread_join(waker);
  for (int i = 0; i < THREADS_NUMBER; i++) {
    klog("Thread:%d runtime:%u.%u sleeptime:%u.%u context switches:%llu", i,
         threads[i]->td_rtime.tv_sec, threads[i]->td_rtime.tv_usec,
         threads[i]->td_slptime.tv_sec, threads[i]->td_slptime.tv_usec,
         threads[i]->td_nctxsw);
    if ((threads[i]->td_rtime.tv_sec == 0 &&
         threads[i]->td_rtime.tv_usec == 0) ||
        (threads[i]->td_slptime.tv_sec == 0 &&
         threads[i]->td_slptime.tv_usec == 0))
      return KTEST_FAILURE;
  }
  return KTEST_SUCCESS;
}

KTEST_ADD(thread_stats_nop, test_thread_stats_nop, 0);
KTEST_ADD(thread_stats_slp, test_thread_stats_slp, 0);
