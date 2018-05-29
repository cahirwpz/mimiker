#include <ktest.h>
#include <mutex.h>
#include <runq.h>
#include <sched.h>
#include <thread.h>

#define T 5

/* Test of turnstile_adjust function.
 *
 * We block T threads on a mutex, then change their priorities via
 * sched_set_prio (which calls turnstile_adjust).
 * Test passes if threads are properly sorted on turnstile ts_blocked list. */

typedef TAILQ_HEAD(threadqueue, thread) threadqueue_t;

static prio_t new_priorities[T] = {3, 1, 2, 0, 1};
static mtx_t ts_adj_mtx = MTX_INITIALIZER(MTX_DEF);
static volatile int stopped;
static thread_t *threads[T];

static void routine(void *_arg) {
  WITH_NO_PREEMPTION {
    stopped = 1;
    mtx_lock(&ts_adj_mtx);
  }

  mtx_unlock(&ts_adj_mtx);
}

static int lockq_sorted_forw(thread_t *td) {
  if (td == NULL)
    return 1;
  else {
    thread_t *next = TAILQ_NEXT(td, td_blockedq);
    if (next != NULL && next->td_prio > td->td_prio)
      return 0;
    else
      return lockq_sorted_forw(next);
  }
}

static int lockq_sorted_back(thread_t *td) {
  if (td == NULL)
    return 1;
  else {
    thread_t *prev = TAILQ_PREV(td, threadqueue, td_blockedq);
    if (prev != NULL && prev->td_prio < td->td_prio)
      return 0;
    else
      return lockq_sorted_back(prev);
  }
}

static int turnstile_sorted(thread_t *td) {
  return lockq_sorted_back(td) && lockq_sorted_forw(td);
}

static int test_turnstile_adjust(void) {
  for (int i = 0; i < T; i++) {
    char name[20];
    snprintf(name, sizeof(name), "td%d", i);
    threads[i] = thread_create(name, routine, NULL);
  }

  mtx_lock(&ts_adj_mtx);

  for (int i = 0; i < T; i++) {
    stopped = 0;
    sched_add(threads[i]);
    while (stopped != 1)
      thread_yield();
  }

  /* Now all the threads should be blocked on the mutex. */

  for (int i = 0; i < T; i++) {
    WITH_SPINLOCK(threads[i]->td_spin) {
      sched_set_prio(threads[i], new_priorities[i]);
    }

    assert(turnstile_sorted(threads[i]));
  }

  mtx_unlock(&ts_adj_mtx);

  for (int i = 0; i < T; i++)
    thread_join(threads[i]);

  return KTEST_SUCCESS;
}

KTEST_ADD(turnstile_adjust, test_turnstile_adjust, 0);
