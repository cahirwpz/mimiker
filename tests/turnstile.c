#include <ktest.h>
#include <mutex.h>
#include <klog.h> // potem to usunąć
#include <runq.h>
#include <sched.h>
#include <thread.h>

#define T 3

static mtx_t mtx = MTX_INITIALIZER(MTX_DEF);
static thread_t *td[T];
static volatile bool high_mutex_acquired = 0;

typedef enum {
  /* Set priorities to multiplies of RunQueue_PriorityPerQueue
   * so that each priority matches different run queue.
   */
  LOW = 0,
  MED = RQ_PPQ,
  HIGH = 2 * RQ_PPQ
} prio_t;

static void high_prio_task(void *arg) {
  assert(mtx.m_owner == td[0]);

  klog("high przed mtx");
  WITH_MTX_LOCK (&mtx) {
    klog("high w mtx");
    high_mutex_acquired = 1;
  }
  klog("high po mtx");
}

static void med_prio_task(void *arg) {
  klog("med");
  while (high_mutex_acquired == 0)
    ; // loop
}

static void low_prio_task(void *arg) {
  /* As for now, td1, td2 and td3 have artificial priorities (HIGH, LOW, LOW)
   * to ensure that this code runs first. Now we can set priorities to their
   * real values (LOW, MED, HIGH).
   */

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
  }

  klog("low przed mtx");
  WITH_MTX_LOCK (&mtx) {
    assert(high_mutex_acquired == 0);
    thread_yield();

    /* Our priority should've been raised. */
    assert(td[0]->td_prio == HIGH);
  }
  klog("low po mtx");

  assert(high_mutex_acquired == 1);
  /* And lowered to base priority. */
  assert(td[0]->td_prio == td[0]->td_base_prio);
  /* Which is LOW. */
  assert(td[0]->td_base_prio == LOW);
}

static int test_mutex_priority_inversion(void) {
  td[0] = thread_create("td1", low_prio_task, NULL);
  td[1] = thread_create("td2", med_prio_task, NULL);
  td[2] = thread_create("td3", high_prio_task, NULL);

  /* We want to ensure that td1 (low_prio_task) will run as the first one
   * and lock mtx.
   */

  WITH_SPINLOCK(td[0]->td_spin) {
    sched_lend_prio(td[0], HIGH);
  }

  assert(td[1]->td_prio == LOW);
  assert(td[2]->td_prio == LOW);

  WITH_NO_PREEMPTION {
    for (int i = 0; i < T; i++)
      sched_add(td[i]);
  }

  thread_join(td[1]);

  return 0;
}

KTEST_ADD(turnstile, test_mutex_priority_inversion, 0);
