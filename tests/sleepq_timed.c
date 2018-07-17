#include <sleepq.h>
#include <ktest.h>
#include <time.h>
#include <thread.h>
#include <sched.h>
#include <runq.h>

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

static volatile int timed_received;
static volatile int signaled_received;
static volatile int signaled_sent;

static thread_t *waiters[THREADS];
static thread_t *waker;

static void waiter_routine(void *_arg) {
  systime_t before_sleep = getsystime();
  sq_wakeup_t status = sleepq_wait_timed(&wchan, __caller(0), SLEEP_TICKS);
  systime_t after_sleep = getsystime();
  systime_t diff = after_sleep - before_sleep;

  if (status == SQ_TIMEOUT) {
    timed_received++;
    assert(diff >= SLEEP_TICKS);
  } else if (status == SQ_NORMAL) {
    signaled_received++;
  } else {
    panic("Got unexpected wakeup status");
  }
}

static void waker_routine(void *_arg) {
  /* try to wake up half of the threads before timeout */
  for (int i = 0; i < THREADS / 2; i++) {
    bool status = sleepq_signal(&wchan);
    if (status)
      signaled_sent++;
  }
}

static int test_sleepq_timed(void) {
  timed_received = 0;
  signaled_received = 0;
  signaled_sent = 0;

  waker = thread_create("waker", waker_routine, NULL);
  for (int i = 0; i < THREADS; i++) {
    char name[20];
    snprintf(name, sizeof(name), "waiter%d", i);
    waiters[i] = thread_create(name, waiter_routine, NULL);
  }

  for (int i = 0; i < THREADS; i++) {
    WITH_SPINLOCK(waiters[i]->td_spin) {
      sched_set_prio(waiters[i], RQ_PPQ);
    }
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
