#include <sleepq.h>
#include <runq.h>
#include <ktest.h>
#include <sched.h>

static int some_val;
static thread_t *td_waiter;
static thread_t *td_waker;

static void waiter_routine(void *_arg) {
  slp_wakeup_t rsn = sleepq_wait_flg(&some_val, __caller(0), SLPF_INT);

  assert(rsn == SLP_WKP_INT);
  // TODO handle some signal? (but there is none because see below)
}

static void waker_routine(void *_arg) {
  // kernel threads don't have a process :/
  // sig_send(td_waiter->td_proc, SIGINT);
  sleepq_signal_thread(td_waiter, SLP_WKP_INT);
}

static int test_main(void) {
  td_waiter = thread_create("td-waiter", waiter_routine, NULL);
  td_waker = thread_create("td-waker", waker_routine, NULL);

  WITH_SPINLOCK(td_waiter->td_spin) {
    sched_set_prio(td_waiter, RQ_PPQ);
  }

  sched_add(td_waiter);
  sched_add(td_waker);

  thread_join(td_waiter);
  thread_join(td_waker);

  return KTEST_SUCCESS;
}

KTEST_ADD(sleepq_int, test_main, 0);
