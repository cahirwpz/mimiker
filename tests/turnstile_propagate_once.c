#include <ktest.h>
#include <mutex.h>
#include <runq.h>
#include <sched.h>
#include <thread.h>

#define T 3

/* This test checks single priority propagation using mutexes.
 *
 * High priority task (td2) tries to acquire mutex and blocks, but firstly lends
 * its priority to low priority task (td0) that owns the mutex. */

static mtx_t *mtx = &MTX_INITIALIZER(MTX_DEF);
static thread_t *td[T];
static volatile bool high_prio_mtx_acquired;

static void assert_priorities(prio_t p0, prio_t p1, prio_t p2) {
  assert(T >= 3);
  assert(td[0]->td_prio == p0);
  assert(td[1]->td_prio == p1);
  assert(td[2]->td_prio == p2);
}

static void lend_prio(thread_t *td, prio_t prio) {
  WITH_SPINLOCK(td->td_spin) {
    sched_lend_prio(td, prio);
  }
}

static void unlend_prio(thread_t *td, prio_t prio) {
  WITH_SPINLOCK(td->td_spin) {
    sched_unlend_prio(td, prio);
  }
}

enum {
  /* Priorities are multiplies of RunQueue_PriorityPerQueue
   * so that each priority matches different run queue. */
  LOW = 0,
  MED = RQ_PPQ,
  HIGH = 2 * RQ_PPQ
};

/* code executed by td0 */
static void low_prio_task(void *arg) {
  /* As for now, td0, td1 and td2 have artificial priorities (HIGH, LOW, LOW)
   * to ensure that this code runs first. Now we can set priorities to their
   * real values (LOW, MED, HIGH). */

  WITH_NO_PREEMPTION {
    unlend_prio(td[0], LOW);
    lend_prio(td[1], MED);
    lend_prio(td[2], HIGH);

    WITH_MTX_LOCK (mtx) {
      /* We were the first to take the mutex. */
      assert(high_prio_mtx_acquired == 0);
      thread_yield();

      /* Our priority should've been raised. */
      assert(thread_self()->td_prio == HIGH);
      assert(td_is_borrowing(thread_self()));

      /* And high priority task is still waiting. */
      assert(high_prio_mtx_acquired == 0);
    }
  }

  /* Check that exiting mutex (lowering our priority) preempted us and
   * ran high priority task. */
  assert(high_prio_mtx_acquired == 1);

  assert(!td_is_borrowing(thread_self()));
  assert_priorities(LOW, MED, HIGH);
}

/* code executed by td1 */
static void med_prio_task(void *arg) {
  /* Without turnstile mechanism med_prio_task would run
   * before high_prio_task gets mtx (priority inversion). */
  assert(high_prio_mtx_acquired);
}

/* code executed by td2 */
static void high_prio_task(void *arg) {
  assert(mtx->m_owner == td[0]);

  WITH_MTX_LOCK (mtx)
    high_prio_mtx_acquired = 1;
}

static int test_turnstile_propagate_once(void) {
  high_prio_mtx_acquired = 0;

  td[0] = thread_create("td0", low_prio_task, NULL);
  td[1] = thread_create("td1", med_prio_task, NULL);
  td[2] = thread_create("td2", high_prio_task, NULL);

  /* We want to ensure that td0 will run as the first one and lock mtx. */
  WITH_NO_PREEMPTION {
    for (int i = 0; i < T; i++)
      sched_add(td[i]);

    lend_prio(td[0], HIGH);
    assert_priorities(HIGH, LOW, LOW);
  }

  for (int i = 0; i < T; i++)
    thread_join(td[i]);

  return KTEST_SUCCESS;
}

KTEST_ADD(turnstile_propagate_once, test_turnstile_propagate_once,
          KTEST_FLAG_BROKEN);
