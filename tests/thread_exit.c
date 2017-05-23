#include <stdc.h>
#include <clock.h>
#include <thread.h>
#include <sched.h>
#include <ktest.h>

static timeval_t exit_time[] = {TIMEVAL(0.100), TIMEVAL(0.200), TIMEVAL(0.150)};
static timeval_t start;

/* TODO: callout + sleepq, once we've implemented callout_schedule. */
static void test_thread(void *p) {
  timeval_t *e = (timeval_t *)p;
  while (1) {
    timeval_t now = clock_get();
    timeval_t diff;
    timeval_sub(&now, &start, &diff);
    if (timeval_cmp(&diff, e, >))
      thread_exit(0);
    else
      sched_yield();
  }
}

/* This tests both thread_join as well as thread_exit. */
static int test_thread_join() {
  thread_t *t1 = thread_create("test-thread-1", test_thread, &exit_time[0]);
  thread_t *t2 = thread_create("test-thread-2", test_thread, &exit_time[1]);
  thread_t *t3 = thread_create("test-thread-3", test_thread, &exit_time[2]);

  tid_t t1_id = t1->td_tid;
  tid_t t2_id = t2->td_tid;
  tid_t t3_id = t3->td_tid;

  start = clock_get();
  sched_add(t1);
  sched_add(t2);
  sched_add(t3);

  thread_join(t1);
  thread_join(t2);
  thread_join(t3);

  /* Ensure that all zombie threads were reaped. */
  thread_reap();

  /* Checks that the threads no longer exist. */
  assert(thread_get_by_tid(t1_id) == NULL);
  assert(thread_get_by_tid(t2_id) == NULL);
  assert(thread_get_by_tid(t3_id) == NULL);

  return KTEST_SUCCESS;
}

KTEST_ADD(thread_join, test_thread_join, 0);
