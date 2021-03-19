#include <sys/mimiker.h>
#include <sys/ktest.h>
#include <sys/klog.h>
#include <sys/mutex.h>
#include <sys/runq.h>
#include <sys/sched.h>
#include <sys/thread.h>

/* Length of the propagation chain. */
#define T 4

/* This test checks chain priority propagation using mutexes.
 *
 * We create one `starter` thread and T `propagator` threads. Each propagator
 * acquires unowned mutex and tries to acquire mutex owned by previous
 * propagator causing chain propagation to all previous propagator threads. */

static mtx_t mtx[T + 1];
/* For simpler code: propagator[0] = starter */
static thread_t *propagator[T + 1];
static thread_t *starter;

/* n <- [0..T] */
static int propagator_prio(int n) {
  /* HACK: Priorities differ by RQ_PPQ so that threads occupy different runq. */
  return prio_kthread(0) + (T - n) * RQ_PPQ;
}

static bool td_is_blocked_on_mtx(thread_t *td, mtx_t *m) {
  assert(td_is_blocked(td));
  return td->td_wchan == m;
}

/* - n <- [1..T]
 * - mtx[n-1] is already owned by propagator[n-1] (it was run earlier)
 * - propagator[n] acquires mtx[n] and blocks on mtx[n-1] causing
 *   priority propagation to propagator[0..n-1]
 */
static void propagator_routine(int n) {
  assert(prio_eq(thread_self()->td_prio, propagator_prio(n)));
  assert(mtx_owner(&mtx[n]) == NULL);
  WITH_MTX_LOCK (&mtx[n]) {
    assert(mtx_owner(&mtx[n - 1]) == propagator[n - 1]);
    WITH_MTX_LOCK (&mtx[n - 1]) {
      // Nothing interesting here.
    }
  }
  assert(!td_is_borrowing(thread_self()));
  assert(prio_eq(thread_self()->td_prio, propagator_prio(n)));
}

static void starter_routine(void *_arg) {
  assert(mtx_owner(&mtx[0]) == NULL);
  WITH_MTX_LOCK (&mtx[0]) {
    for (int i = 1; i <= T; i++) {
      WITH_NO_PREEMPTION {
        sched_add(propagator[i]);
        assert(prio_eq(thread_self()->td_prio, propagator_prio(i - 1)));
      }

      assert(td_is_blocked_on_mtx(propagator[i], &mtx[i - 1]));
      assert(mtx_owner(&mtx[i - 1]) == propagator[i - 1]);

      /* Check if the priorities have propagated correctly. */
      for (int j = 0; j < i; j++) {
        assert(prio_eq(propagator[j]->td_prio, propagator_prio(i)));
        assert(td_is_borrowing(propagator[j]));
      }
    }
  }
  assert(prio_eq(thread_self()->td_prio, propagator_prio(0)));
  assert(!td_is_borrowing(thread_self()));
}

static int test_turnstile_propagate_many(void) {
  for (int i = 0; i < T + 1; i++)
    mtx_init(&mtx[i], 0);

  for (int i = 1; i <= T; i++) {
    char name[20];
    snprintf(name, sizeof(name), "test-turnstile-prop-%d", i);
    propagator[i] = thread_create(name, (void *)(intptr_t)propagator_routine,
                                  (void *)(intptr_t)i, propagator_prio(i));
  }

  starter = thread_create("test-turnstile-starter", starter_routine, NULL,
                          propagator_prio(0));
  propagator[0] = starter;
  sched_add(starter);

  for (int i = 0; i < T + 1; i++)
    thread_join(propagator[i]);

  return KTEST_SUCCESS;
}

KTEST_ADD(turnstile_propagate_many, test_turnstile_propagate_many, 0);
