#include <sys/klog.h>
#include <sys/libkern.h>
#include <sys/time.h>
#include <sys/thread.h>
#include <sys/sched.h>
#include <sys/ktest.h>

static bintime_t start;

/* TODO: callout + sleepq, once we've implemented callout_schedule. */
static void test_thread(void *p) {
  bintime_t *e = (bintime_t *)p;
  while (1) {
    bintime_t now = binuptime();
    bintime_sub(&now, &start);
    if (bintime_cmp(&now, e, >))
      thread_exit();
    else
      thread_yield();
  }
}

/* This tests both thread_join as well as thread_exit. */
static int test_thread_join(void) {
  bintime_t exit_time[] = {BINTIME(0.100), BINTIME(0.200), BINTIME(0.150)};
  thread_t *t1 = thread_create("test-thread-exit-1", test_thread, &exit_time[0],
                               prio_kthread(0));
  thread_t *t2 = thread_create("test-thread-exit-2", test_thread, &exit_time[1],
                               prio_kthread(0));
  thread_t *t3 = thread_create("test-thread-exit-3", test_thread, &exit_time[2],
                               prio_kthread(0));

  tid_t t1_id = t1->td_tid;
  tid_t t2_id = t2->td_tid;
  tid_t t3_id = t3->td_tid;

  start = binuptime();
  sched_add(t1);
  sched_add(t2);
  sched_add(t3);

  thread_join(t1);
  thread_join(t2);
  thread_join(t3);

  /* Ensure that all zombie threads were reaped. */
  thread_reap();

  /* Checks that the threads no longer exist. */
  assert(thread_find(t1_id) == NULL);
  assert(thread_find(t2_id) == NULL);
  assert(thread_find(t3_id) == NULL);

  return KTEST_SUCCESS;
}

KTEST_ADD(thread_join, test_thread_join, 0);
