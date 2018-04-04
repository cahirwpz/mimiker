#include <ktest.h>
#include <mutex.h>
#include <runq.h>
#include <sched.h>
#include <thread.h>

#define T 3

static mtx_t mtx = MTX_INITIALIZER(MTX_DEF);
static thread_t *td[T];

static enum prio {
  /* Set priorities to multiplies of RunQueue_PriorityPerQueue
   * so that each priority matches different run queue.
   */
  LOW = 0,
  MED = RQ_PPQ,
  HIGH = 2 * RQ_PPQ
} last_prio;

static void high_prio_task(void *arg) {
  assert(mtx.m_owner == td[2]);

  mtx_lock(&mtx);
  mtx_unlock(&mtx);

  last_prio = HIGH;
}

static void med_prio_task(void *arg) {
  /* Some work of medium importance that should be done
   * after high_prio_task.
   */

  last_prio = MED;
}

static void low_prio_task(void *arg) {
  /* As for now, td1, td2 and td3 have artificial priorities (LOW, LOW, HIGH)
   * to ensure that this code runs first. Now we can set priorities to their
   * real values (HIGH, MED, LOW).
   */
  td[0]->td_prio = HIGH;
  td[1]->td_prio = MED;
  td[2]->td_prio = LOW;

  mtx_lock(&mtx);
  /* Yield, so that high priority task will have to wait for us. */
  thread_yield();
  mtx_unlock(&mtx);

  last_prio = LOW;
}

static int test_mutex_priority_inversion(void) {
  td[0] = thread_create("td1", high_prio_task, NULL);
  td[1] = thread_create("td2", med_prio_task, NULL);
  td[2] = thread_create("td3", low_prio_task, NULL);

  /* We want to ensure that td3 (low_prio_task) will run as the first one
   * and lock mtx.
   */
  td[0]->td_prio = LOW;
  td[1]->td_prio = LOW;
  td[2]->td_prio = HIGH;

  for (int i = 0; i < T; i++)
    sched_add(td[i]);

  for (int i = 0; i < T; i++)
    thread_join(td[i]);

  /* High priority task should've propagated its priority to low priority
   * task and the last one to run should've been medium priority task.
   */
  assert(last_prio == MED);

  return 0;
}

KTEST_ADD(turnstile, test_mutex_priority_inversion, 0);