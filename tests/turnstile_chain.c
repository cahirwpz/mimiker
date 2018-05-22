#include <ktest.h>
#include <klog.h>
#include <mutex.h>
#include <runq.h>
#include <sched.h>
#include <thread.h>

/* Length of the propagation chain. */
#define T 4

static mtx_t mtx[T + 1];
/* For simpler code: propagator[0] = starter */
static thread_t *propagator[T + 1];
static thread_t *starter;

static void set_prio(thread_t *td, prio_t prio) {
  WITH_SPINLOCK(td->td_spin) {
    sched_set_prio(td, prio);
  }
}

// n <- [0..T]
static int propagator_prio(int n) {
  return n * RQ_PPQ;
}

/* - n <- [1..T]
 * - mtx[n-1] is already owned by propagator[n-1] (it was run earlier)
 * - propagator[n] acquires mtx[n] and blocks on mtx[n-1] causing
 *   priority propagation to propagator[0..n-1]
 */
static void propagator_routine(int n) {
  assert(thread_self()->td_prio == propagator_prio(n));
  assert(mtx_owner(&mtx[n]) == NULL);
  WITH_MTX_LOCK (&mtx[n]) {
    assert(mtx_owner(&mtx[n - 1]) == propagator[n - 1]);
    WITH_MTX_LOCK (&mtx[n - 1]) {
      /* Nothing interesting here. */
    }
  }
  assert(!td_is_borrowing(thread_self()));
  assert(thread_self()->td_prio == propagator_prio(n));
}

static void starter_routine(void *_arg) {
  assert(mtx_owner(&mtx[0]) == NULL);
  WITH_MTX_LOCK (&mtx[0]) {
    for (int i = 1; i <= T; i++) {
      WITH_NO_PREEMPTION {
        set_prio(propagator[i], propagator_prio(i));
        sched_add(propagator[i]);
        assert(thread_self()->td_prio == propagator_prio(i - 1));
      }

      /* propagator[i] waits for mtx[i-1] (owned by propagator[i-1]) */

      /* Check if the priorities have propagated correctly. */
      for (int j = 0; j < i; j++) {
        assert(propagator[j]->td_prio == propagator_prio(i));
        assert(td_is_borrowing(propagator[j]));
      }
    }
  }
  assert(thread_self()->td_prio == propagator_prio(0));
  assert(!td_is_borrowing(thread_self()));
}

static int test_main(void) {
  klog_setmask(KL_MASK(KL_TEST) | KL_MASK(KL_THREAD));

  for (int i = 0; i < T + 1; i++)
    mtx[i] = MTX_INITIALIZER(MTX_DEF);

  for (int i = 1; i <= T; i++) {
    char name[20];
    snprintf(name, sizeof(name), "prop%d", i);
    propagator[i] =
      thread_create(name, (void (*)(void *))propagator_routine, (void *)i);
  }
  starter = thread_create("starter", starter_routine, NULL);

  propagator[0] = starter;

  set_prio(starter, propagator_prio(0));
  sched_add(starter);

  for (int i = 0; i < T + 1; i++)
    thread_join(propagator[i]);

  klog_setmask(KL_ALL);

  return KTEST_SUCCESS;
}

KTEST_ADD(turnstile_chain_propagation, test_main, 0);
