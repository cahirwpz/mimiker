#include <ktest.h>
#include <mutex.h>
#include <runq.h>
#include <sched.h>
#include <thread.h>

#define T 3

#define assert_priorities(p0, p1, p2)                                          \
  assert(T >= 3 && td[0]->td_prio == p0 && td[1]->td_prio == p1 &&             \
         td[2]->td_prio == p2);

#define lend_prio(td, prio)                                                    \
  WITH_SPINLOCK(td->td_spin) {                                                 \
    sched_lend_prio(td, prio);                                                 \
  }

#define unlend_prio(td, prio)                                                  \
  WITH_SPINLOCK(td->td_spin) {                                                 \
    sched_unlend_prio(td, prio);                                               \
  }

static mtx_t *mtx = &MTX_INITIALIZER(MTX_DEF);
static thread_t *td[T];
static volatile bool high_prio_mtx_acquired;

enum {
  /* Priorities are multiplies of RunQueue_PriorityPerQueue
   * so that each priority matches different run queue. */
  LOW = 0,
  MED = RQ_PPQ,
  HIGH = 2 * RQ_PPQ
};

static void high_prio_task(void *arg) {
  assert(mtx->m_owner == td[0]);

  WITH_MTX_LOCK (mtx)
    high_prio_mtx_acquired = 1;
}

static void med_prio_task(void *arg) {
  /* Without turnstile mechanism this assert would fail. */
  assert(high_prio_mtx_acquired);
}

static void low_prio_task(void *arg) {
  /* As for now, td0, td1 and td2 have artificial priorities (HIGH, LOW, LOW)
   * to ensure that this code runs first. Now we can set priorities to their
   * real values (LOW, MED, HIGH). */

  WITH_NO_PREEMPTION {
    unlend_prio(td[0], LOW);
    lend_prio(td[1], MED);
    lend_prio(td[2], HIGH);

    WITH_MTX_LOCK (mtx) {
      /* High priority task didn't run yet. */
      assert(high_prio_mtx_acquired == 0);
      thread_yield();

      /* Our priority should've been raised. */
      assert(thread_self()->td_prio == HIGH);
      assert(thread_self()->td_flags & TDF_BORROWING);

      /* And high priority task is still waiting. */
      assert(high_prio_mtx_acquired == 0);
    }
  }

  /* Check that exiting mutex (lowering our priority) preempted us and
   * ran high priority task. */
  assert(high_prio_mtx_acquired == 1);

  assert(!(thread_self()->td_flags & TDF_BORROWING));
  assert_priorities(LOW, MED, HIGH);
}

static int test_mutex_priority_inversion(void) {
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

KTEST_ADD(turnstile_simple, test_mutex_priority_inversion, 0);
