#include <ktest.h>
#include <mutex.h>
#include <runq.h>
#include <sched.h>
#include <thread.h>

#define T 3

static mtx_t *mtx = &MTX_INITIALIZER(MTX_DEF);
static thread_t *td[T];
static volatile bool high_prio_mtx_acquired;

typedef enum {
  /* Set priorities to multiplies of RunQueue_PriorityPerQueue
   * so that each priority matches different run queue. */
  LOW = 0,
  MED = RQ_PPQ,
  HIGH = 2 * RQ_PPQ
} prio_t;

static void high_prio_task(void *arg) {
  assert(mtx->m_owner == td[0]);

  WITH_MTX_LOCK (mtx)
    high_prio_mtx_acquired = 1;
}

static void med_prio_task(void *arg) {
  assert(high_prio_mtx_acquired);
}

static void low_prio_task(void *arg) {
  /* As for now, td0, td1 and td2 have artificial priorities (HIGH, LOW, LOW)
   * to ensure that this code runs first. Now we can set priorities to their
   * real values (LOW, MED, HIGH). */

  WITH_NO_PREEMPTION {
    WITH_SPINLOCK(td[0]->td_spin) {
      sched_unlend_prio(td[0], LOW);
    }
    WITH_SPINLOCK(td[1]->td_spin) {
      sched_lend_prio(td[1], MED);
    }
    WITH_SPINLOCK(td[2]->td_spin) {
      sched_lend_prio(td[2], HIGH);
    }

    WITH_MTX_LOCK (mtx) {
      /* High priority task didn't run yet. */
      assert(high_prio_mtx_acquired == 0);
      thread_yield();

      /* Our priority should've been raised. */
      assert(td[0]->td_prio == HIGH);
      assert(td[0]->td_flags & TDF_BORROWING);

      /* And high priority task is still waiting. */
      assert(high_prio_mtx_acquired == 0);
    }
  }

  /* Check that exiting mutex (lowering our priority) preempted us and
   * ran high priority task. */
  assert(high_prio_mtx_acquired == 1);

  /* We don't borrow anymore. */
  assert(!(td[0]->td_flags & TDF_BORROWING));
  /* Our priority was lowered to base priority. */
  assert(td[0]->td_prio == td[0]->td_base_prio);
  /* Which is LOW. */
  assert(td[0]->td_base_prio == LOW);

  /* And we didn't spoil other priorities. */
  assert(td[1]->td_prio == MED);
  assert(td[2]->td_prio == HIGH);
}

static int test_mutex_priority_inversion(void) {
  high_prio_mtx_acquired = 0;

  td[0] = thread_create("td0", low_prio_task, NULL);
  td[1] = thread_create("td1", med_prio_task, NULL);
  td[2] = thread_create("td2", high_prio_task, NULL);

  /* We want to ensure that td0 (low_prio_task) will run as the first one
   * and lock mtx. */
  WITH_NO_PREEMPTION {
    for (int i = 0; i < T; i++)
      sched_add(td[i]);

    WITH_SPINLOCK(td[0]->td_spin) {
      sched_lend_prio(td[0], HIGH);
    }

    assert(td[0]->td_prio == HIGH);
    assert(td[1]->td_prio == LOW);
    assert(td[2]->td_prio == LOW);
  }

  for (int i = 0; i < T; i++)
    thread_join(td[i]);

  return KTEST_SUCCESS;
}

KTEST_ADD(turnstile, test_mutex_priority_inversion, 0);
