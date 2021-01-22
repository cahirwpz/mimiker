#include <sys/mimiker.h>
#include <sys/sleepq.h>
#include <sys/ktest.h>
#include <sys/time.h>
#include <sys/thread.h>
#include <sys/sched.h>
#include <sys/errno.h>
#include <sys/runq.h>

/* `waiters[i]` wait with timeout on `&wchan`.
 * `waker` wakes up half of them with `sleepq_signal(&wchan)`.
 * We count the number of successful `sleepq_signal` from the perspective of
 * the waker and number of received `SQ_TIMED` and `SQ_NORMAL` from the
 * perspective of the waiters.
 *
 * This test doesn't check if timed sleep is also indeed interruptible.
 */

/* just to get some unique address for sleepq */
static int wchan;

#define THREADS 6
#define SLEEP_TICKS 4

static volatile atomic_int timed_received;
static volatile atomic_int signaled_received;
static volatile atomic_int signaled_sent;

static thread_t *waiters[THREADS];
static thread_t *waker;

static void waiter_routine(void *_arg) {
  systime_t before_sleep = getsystime();
  int status = sleepq_wait_timed(&wchan, __caller(0), SLEEP_TICKS);
  systime_t after_sleep = getsystime();
  systime_t diff = after_sleep - before_sleep;

  if (status == ETIMEDOUT) {
    atomic_fetch_add(&timed_received, 1);
    assert(diff >= SLEEP_TICKS);
  } else if (status == 0) {
    atomic_fetch_add(&signaled_received, 1);
  } else {
    panic("Got unexpected wakeup status: %d!", status);
  }
}

static void waker_routine(void *_arg) {
  /* try to wake up half of the threads before timeout */
  for (int i = 0; i < THREADS / 2; i++) {
    bool status = sleepq_signal(&wchan);
    if (status)
      atomic_fetch_add(&signaled_sent, 1);
  }
}

static int test_sleepq_timed(void) {
  timed_received = 0;
  signaled_received = 0;
  signaled_sent = 0;

  /* HACK: Priorities differ by RQ_PPQ so that threads occupy different runq. */
  waker =
    thread_create("test-sleepq-waker", waker_routine, NULL, prio_kthread(0));
  for (int i = 0; i < THREADS; i++) {
    char name[20];
    snprintf(name, sizeof(name), "test-sleepq-waiter-%d", i);
    waiters[i] =
      thread_create(name, waiter_routine, NULL, prio_kthread(0) + RQ_PPQ);
  }

  WITH_NO_PREEMPTION {
    for (int i = 0; i < THREADS; i++)
      sched_add(waiters[i]);
    sched_add(waker);
  }

  thread_join(waker);
  for (int i = 0; i < THREADS; i++)
    thread_join(waiters[i]);

  assert(signaled_received == signaled_sent);
  assert(signaled_received + timed_received == THREADS);
  /* At most floor(T/2) threads were woken by sleepq_signal */
  assert(timed_received >= (THREADS + 1) / 2);

  return KTEST_SUCCESS;
}

KTEST_ADD(sleepq_timed, test_sleepq_timed, 0);
