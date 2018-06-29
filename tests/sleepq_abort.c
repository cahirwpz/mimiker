#include <sleepq.h>
#include <runq.h>
#include <ktest.h>
#include <sched.h>

#define T 6

#define RAND_COUNT 13
/* obtained with totally fair d6 rolls (1-3 true; 4-6 false) */
static bool kinda_random_values[RAND_COUNT] = {true,  true,  false, true, false,
                                               false, true,  false, true, true,
                                               false, false, false};

/* Puts one thread to sleep and aborts it with a second thread. */
static int test_sleepq_abort_simple(void);
/* Runs a couple of threads that sleep on a specified channel and one thread
 * that kinda randomly wakes them up or aborts their sleep. Keeps track of
 * the number of threads that were interrupted and woken up normally and
 * of the number of apparently successful aborts and regular wake-ups.
 */
static int test_sleepq_abort_mult(void);

/* some random ordering for waiters (values from 0 to T-1) */
static int waiters_ord[T] = {5, 2, 1, 3, 4, 0};
static thread_t *waiters[T];
static thread_t *waker;
/* just to have some unused waiting channel */
static int some_val;
static volatile int wakened_gracefully;
static volatile int interrupted;

/* Waiters have higher priority so that waker will only execute
 * when waiters can't.
 * Therefore there should be only one waiter active at once */
static void waiter_routine(void *_arg) {
  sq_wakeup_t rsn =
    sleepq_wait_abortable(&some_val, __caller(0), SQ_INTERRUPTED);

  if (rsn == SQ_INTERRUPTED)
    interrupted++;
  else if (rsn == SQ_REGULAR)
    wakened_gracefully++;
  else
    assert(false);
}

static void waker_routine(void *_arg) {
  int wakened = 0; /* total */
  int aborted = 0;
  int next_abort = 0;
  int rand_next = 0;

  while (wakened < T) {
    /* this is a terrible way for randomizing variables
     * (especially when it's probably not even needed) */
    bool wake = kinda_random_values[rand_next % RAND_COUNT];
    rand_next++;

    if (wake) {
      bool succ = sleepq_signal(&some_val);
      assert(succ);
      wakened++;
    } else {
      bool succ = false;
      while (!succ) {
        assert(next_abort < T && waiters_ord[next_abort] < T);
        succ = sleepq_abort(waiters[waiters_ord[next_abort]], SQ_INTERRUPTED);
        next_abort++;
      }
      aborted++;
      wakened++;
    }

    WITH_NO_PREEMPTION {
      /* plz run higher priority threads */
    }
  }

  assert(T == wakened);
  assert(interrupted == aborted);
  assert(T == interrupted + wakened_gracefully);
}

static int test_sleepq_abort_mult(void) {
  wakened_gracefully = 0;
  interrupted = 0;

  waker = thread_create("waker", waker_routine, NULL);
  for (int i = 0; i < T; i++) {
    char name[20];
    snprintf(name, sizeof(name), "waiter%d", i);
    waiters[i] = thread_create(name, waiter_routine, NULL);
  }

  for (int i = 0; i < T; i++) {
    WITH_SPINLOCK(waiters[i]->td_spin) {
      sched_set_prio(waiters[i], RQ_PPQ);
    }
  }

  for (int i = 0; i < T; i++)
    sched_add(waiters[i]);
  sched_add(waker);

  /* now only wait until the threads finish */

  thread_join(waker);
  for (int i = 0; i < T; i++)
    thread_join(waiters[i]);

  return KTEST_SUCCESS;
}

static void simple_waker_routine(void *_arg) {
  sleepq_abort(waiters[0], SQ_INTERRUPTED);
}

/* waiter routine is shared with test_mult */
static int test_sleepq_abort_simple(void) {
  waiters[0] = thread_create("waiter", waiter_routine, NULL);
  waker = thread_create("simp-waker", simple_waker_routine, NULL);

  WITH_SPINLOCK(waiters[0]->td_spin) {
    sched_set_prio(waiters[0], RQ_PPQ);
  }

  sched_add(waiters[0]);
  sched_add(waker);

  thread_join(waiters[0]);
  thread_join(waker);

  return KTEST_SUCCESS;
}

KTEST_ADD(sleepq_abort_simple, test_sleepq_abort_simple, 0);
KTEST_ADD(sleepq_abort_mult, test_sleepq_abort_mult, 0);
