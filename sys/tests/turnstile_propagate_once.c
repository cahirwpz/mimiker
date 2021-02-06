#include <sys/mimiker.h>
#include <sys/ktest.h>
#include <sys/mutex.h>
#include <sys/runq.h>
#include <sys/sched.h>
#include <sys/thread.h>

#define T 3

/* This test checks single priority propagation using mutexes.
 *
 * High priority task (td2) tries to acquire mutex and blocks, but firstly lends
 * its priority to low priority task (td0) that owns the mutex. */

static mtx_t *mtx = &MTX_INITIALIZER(mtx, 0);
static thread_t *td[T];
static volatile bool high_prio_mtx_acquired;
static prio_t LOW, MED, HIGH;

static void assert_priorities(prio_t p0, prio_t p1, prio_t p2) {
  assert(T >= 3);
  assert(prio_eq(td[0]->td_prio, p0));
  assert(prio_eq(td[1]->td_prio, p1));
  assert(prio_eq(td[2]->td_prio, p2));
}

/* code executed by td0 */
static void low_prio_task(void *arg) {
  WITH_NO_PREEMPTION {
    sched_add(td[1]);
    sched_add(td[2]);

    WITH_MTX_LOCK (mtx) {
      /* We were the first to take the mutex. */
      assert(high_prio_mtx_acquired == 0);
      thread_yield();

      /* Our priority should've been raised. */
      assert(prio_eq(thread_self()->td_prio, HIGH));
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
  assert(mtx_owner(mtx) == td[0]);

  WITH_MTX_LOCK (mtx)
    high_prio_mtx_acquired = 1;
}

static int test_turnstile_propagate_once(void) {
  high_prio_mtx_acquired = 0;

  /* HACK: Priorities differ by RQ_PPQ so that threads occupy different runq. */
  HIGH = prio_kthread(0);
  MED = HIGH + RQ_PPQ;
  LOW = MED + RQ_PPQ;

  td[0] = thread_create("test-turnstile-low", low_prio_task, NULL, LOW);
  td[1] = thread_create("test-turnstile-med", med_prio_task, NULL, MED);
  td[2] = thread_create("test-turnstile-high", high_prio_task, NULL, HIGH);

  /* We want to ensure that td0 will run as the first one and lock mtx. */
  sched_add(td[0]);

  for (int i = 0; i < T; i++)
    thread_join(td[i]);

  return KTEST_SUCCESS;
}

KTEST_ADD(turnstile_propagate_once, test_turnstile_propagate_once, 0);
