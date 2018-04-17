#include <ktest.h>
#include <mutex.h>
#include <runq.h>
#include <sched.h>
#include <thread.h>

#define T 3

static mtx_t mtx = MTX_INITIALIZER(MTX_DEF);
static thread_t *td[T];

typedef enum {
  /* Set priorities to multiplies of RunQueue_PriorityPerQueue
   * so that each priority matches different run queue.
   */
  LOW = 0,
  MED = RQ_PPQ,
  HIGH = 2 * RQ_PPQ
} prio_t;

static prio_t last_thread; /* which thread ended running as the last one */

static void high_prio_task(void *arg) {
  assert(mtx.m_owner == td[0]);

  mtx_lock(&mtx);
  mtx_unlock(&mtx);

  last_thread = HIGH;
}

static void med_prio_task(void *arg) {
  /* Some work of medium importance that should be done
   * after high_prio_task.
   */

  last_thread = MED;
}

static void low_prio_task(void *arg) {
  /* As for now, td1, td2 and td3 have artificial priorities (HIGH, LOW, LOW)
   * to ensure that this code runs first. Now we can set priorities to their
   * real values (LOW, MED, HIGH).
   */

  WITH_NO_PREEMPTION {
    td_prio_t prios[3] = {LOW, MED, HIGH};

    for (int i = 0; i < 3; i++) {
      spin_acquire(td[i]->td_spin);
      sched_lend_prio(td[i], prios[i]);
      spin_release(td[i]->td_spin);
    }

    mtx_lock(&mtx);
  }

  /* Yield, so that high priority task will have to wait for us. */
  thread_yield();

  /* Our priority should've been raised. */
  assert(td[0]->td_prio == HIGH);

  last_thread = LOW;
  mtx_unlock(&mtx);

  /* And lowered. */
  assert(td[0]->td_prio == LOW);
}

static int test_mutex_priority_inversion(void) {
  td[0] = thread_create("td1", low_prio_task, NULL);
  td[1] = thread_create("td2", med_prio_task, NULL);
  td[2] = thread_create("td3", high_prio_task, NULL);

  /* We want to ensure that td1 (low_prio_task) will run as the first one
   * and lock mtx.
   */

  // nie wiem, czy nie powinienem tutaj założyć mutexa td_lock
  WITH_NO_PREEMPTION {
    td[0]->td_prio = HIGH;
    td[1]->td_prio = LOW;
    td[2]->td_prio = LOW;

    for (int i = 0; i < T; i++)
      sched_add(td[i]);
  }

  for (int i = 0; i < T; i++)
    thread_join(td[i]);

  /* High priority task should've propagated its priority to low priority
   * task and the last one to run should've been medium priority task.
   */
  assert(last_thread == MED);

  return 0;
}

KTEST_ADD(turnstile, test_mutex_priority_inversion, 0);
