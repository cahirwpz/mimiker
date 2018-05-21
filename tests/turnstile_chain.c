#include <ktest.h>
#include <klog.h>
#include <mutex.h>
#include <runq.h>
#include <sched.h>
#include <thread.h>

// length of the propagation chain
#define T 4

// TODO replace asserts about priority borrowing with some accessor
/* TODO consider where we could check restoring priorities
 * I think that all threads should keep HIGH priority until the last propagator
 * has acquired 2nd mutex and then the priorities would be restored to initial
 * ones (propagator_priority(i) for propagator[i]).
 */

static mtx_t mtx[T + 1];
// TODO consider whether this would be any useful
// volatile int mtx_passed[T + 1];

// for simpler code: propagator[0] = starter
static thread_t *propagator[T + 1];
static thread_t *starter;

static void set_prio(thread_t *td, prio_t prio) {
  WITH_SPINLOCK(td->td_spin) {
    sched_set_prio(td, prio);
  }
}

// n <- [0..T]
// TODO maybe rename to separate_prio or something?
static int propagator_prio(int n) {
  // TODO should this remain `n + 1` and not just `n`?
  return (n + 1) * RQ_PPQ;
}

/* n <- [1..T]
 * nth propagator will contest nth mutex and block on (n-1)th mutex (owned
 * by (n-1)th propagator) causing priority propagation to [0..n-1]th propagator
 */
static void propagator_routine(int n) {
  assert(thread_self()->td_prio == propagator_prio(n));
  assert(MTX_OWNER(&mtx[n]) == NULL);
  WITH_MTX_LOCK (&mtx[n]) {

    assert(MTX_OWNER(&mtx[n - 1]) == propagator[n - 1]);
    WITH_MTX_LOCK (&mtx[n - 1]) {
      // nothing here
    }
  }
  assert(thread_self()->td_prio == propagator_prio(n));
}

static void starter_routine(void *_arg) {
  assert(MTX_OWNER(&mtx[0]) == NULL);
  WITH_MTX_LOCK (&mtx[0]) {

    for (int i = 1; i <= T; i++) {
      WITH_NO_PREEMPTION {
        set_prio(propagator[i], propagator_prio(i));
        sched_add(propagator[i]);
        assert(thread_self()->td_prio == propagator_prio(i - 1));
      }

      // check if the priorities have propagated correctly
      for (int j = 0; j < i; j++) {
        assert(propagator[j]->td_prio == propagator_prio(i));
        assert(TD_IS_BORROWING(propagator[j]));
      }
    }
  }
  assert(thread_self()->td_prio == propagator_prio(0));
  assert(!TD_IS_BORROWING(thread_self()));
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
