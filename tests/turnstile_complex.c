#include <ktest.h>
#include <mutex.h>
#include <runq.h>
#include <sched.h>
#include <thread.h>

#define T 4

#define assert_priorities(p0, p1, p2, p3)                                      \
  assert(T >= 4 && td[0]->td_prio == p0 && td[1]->td_prio == p1 &&             \
         td[2]->td_prio == p2 && td[3]->td_prio == p3);

#define lend_prio(td, prio)                                                    \
  WITH_SPINLOCK(td->td_spin) {                                                 \
    sched_lend_prio(td, prio);                                                 \
  }

#define unlend_prio(td, prio)                                                  \
  WITH_SPINLOCK(td->td_spin) {                                                 \
    sched_unlend_prio(td, prio);                                               \
  }

#define set_base_prio(td, prio)                                                \
  WITH_SPINLOCK(td->td_spin) {                                                 \
    td->td_base_prio = prio;                                                   \
  }

static mtx_t *mtx1 = &MTX_INITIALIZER(MTX_DEF);
static mtx_t *mtx2 = &MTX_INITIALIZER(MTX_DEF);
static thread_t *td[T];
static volatile bool low_prio_mtx1_acquired;
static volatile bool med_prio_mtx1_acquired;
static volatile bool high_prio_mtx2_acquired;

enum {
  /* Priorities are multiplies of RunQueue_PriorityPerQueue
   * so that each priority matches different run queue. */
  LOW = 0,
  MED = RQ_PPQ,
  HIGH = 2 * RQ_PPQ
};

/* td3 */
static void high_prio_task(void *arg) {
  WITH_MTX_LOCK (mtx2) {
    /* When we get here, low_prio_task and med_prio_task1 should've
       got their mutexes and all priorities should be in initial state. */
    high_prio_mtx2_acquired = 1;
    assert(low_prio_mtx1_acquired == 1);
    assert(med_prio_mtx1_acquired == 1);

    assert_priorities(LOW, MED, MED, HIGH);
  }
}

/* td1 */
static void med_prio_task1(void *arg) {
  WITH_NO_PREEMPTION {
    WITH_MTX_LOCK (mtx2) {
      WITH_MTX_LOCK (mtx1) {
        /* Td0 (low_prio_task) has released mtx1.
         * Td3 (high_prio_task) is waiting for mtx2. */
        med_prio_mtx1_acquired = 1;
        assert(low_prio_mtx1_acquired == 1);
        assert(high_prio_mtx2_acquired == 0);

        assert(thread_self()->td_prio == HIGH);
        assert(thread_self()->td_flags & TDF_BORROWING);
      }
    }
  }

  /* Enabling preemption should've preempted us and switched to
   * high_prio_task. */
  assert(high_prio_mtx2_acquired == 1);
  assert(!(thread_self()->td_flags & TDF_BORROWING));
  assert_priorities(LOW, MED, MED, HIGH);
}

/* td2 */
static void med_prio_task2(void *arg) {
  /* Without turnstile mechanism this assert would fail. */
  assert(high_prio_mtx2_acquired);
}

/* td0 */
static void low_prio_task(void *arg) {
  WITH_NO_PREEMPTION {
    unlend_prio(td[0], LOW);
    lend_prio(td[1], MED);

    /* Thread td1 (med_prio_task1) will run first after next thread_yield(). */
    assert_priorities(LOW, MED, LOW, LOW);

    WITH_MTX_LOCK (mtx1) {
      low_prio_mtx1_acquired = 1;
      assert(med_prio_mtx1_acquired == 0);
      assert(high_prio_mtx2_acquired == 0);

      thread_yield();

      /* Thread td1 acquired mtx2. Then it tried to acquire mtx1 and
       * lent us MED priority. */
      assert(thread_self()->td_prio == MED);
      assert(thread_self()->td_flags & TDF_BORROWING);

      /* Now we add to the game td2 and td3. */
      lend_prio(td[2], MED);
      lend_prio(td[3], HIGH);
      assert_priorities(MED, MED, MED, HIGH);

      thread_yield();

      /* Thread td3 tried to acquire mtx2 (which is in td1's possession).
       * Td3 lent td1 HIGH priority. We also got HIGH priority as chain
       * propagation happened because td1 is blocked on mtx1, which we
       * possess. */
      assert(thread_self()->td_prio == HIGH);
      assert(thread_self()->td_flags & TDF_BORROWING);
      assert_priorities(HIGH, HIGH, MED, HIGH);

      assert(med_prio_mtx1_acquired == 0);
      assert(high_prio_mtx2_acquired == 0);
    }
  }

  /* Enabling preemption should've preempted us and switched to
   * higher priority tasks. Our priority should've been restored. */
  assert(!(thread_self()->td_flags & TDF_BORROWING));
  assert(med_prio_mtx1_acquired == 1);
  assert(high_prio_mtx2_acquired == 1);

  assert_priorities(LOW, MED, MED, HIGH);
}

static int test_mutex_priority_inversion(void) {
  low_prio_mtx1_acquired = 0;
  med_prio_mtx1_acquired = 0;
  high_prio_mtx2_acquired = 0;

  td[0] = thread_create("td0", low_prio_task, NULL);
  td[1] = thread_create("td1", med_prio_task1, NULL);
  td[2] = thread_create("td2", med_prio_task2, NULL);
  td[3] = thread_create("td3", high_prio_task, NULL);

  WITH_NO_PREEMPTION {
    for (int i = 0; i < T; i++)
      sched_add(td[i]);

    /* Thread td0 should run as the first one. */
    lend_prio(td[0], HIGH);
    assert_priorities(HIGH, LOW, LOW, LOW);

    /* We set base priority to later check that td1's priority
     * is restored to MED when priority propagation is over. */
    set_base_prio(td[1], MED);
  }

  for (int i = 0; i < T; i++)
    thread_join(td[i]);

  return KTEST_SUCCESS;
}

KTEST_ADD(turnstile_complex, test_mutex_priority_inversion, 0);
