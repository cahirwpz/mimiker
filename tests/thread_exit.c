#include <stdc.h>
#include <clock.h>
#include <thread.h>
#include <sched.h>
#include <ktest.h>

int exit_time[] = {100, 200, 150};
realtime_t start;

/* TODO: callout + sleepq */
static void test_thread(void *p) {
  int e = *(int *)p;
  while (1) {
    realtime_t tdiff = clock_get() - start;
    if (tdiff > e)
      thread_exit(0);
    else
      sched_yield();
  }
}

static int test_thread_join() {
  thread_t *t1 = thread_create("test-thread-1", test_thread, &exit_time[0]);
  thread_t *t2 = thread_create("test-thread-2", test_thread, &exit_time[1]);
  thread_t *t3 = thread_create("test-thread-3", test_thread, &exit_time[2]);

  start = clock_get();
  sched_add(t1);
  sched_add(t2);
  sched_add(t3);

  thread_join(t1);
  thread_join(t2);
  thread_join(t3);

  return KTEST_SUCCESS;
}

KTEST_ADD(thread_join, test_thread_join, 0);
