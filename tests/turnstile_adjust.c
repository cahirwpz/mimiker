#include <ktest.h>
#include <mutex.h>
#include <runq.h>
#include <sched.h>
#include <thread.h>

typedef TAILQ_HEAD(threadqueue, thread) threadqueue_t;

#define T 5
// all the priorities must be from one runqueue
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

static int turnstileq_sorted_forw(thread_t *td) {
  if (td == NULL)
    return 1;
  else {
    thread_t *next = TAILQ_NEXT(td, td_turnstileq);
    if (next != NULL && next->td_prio > td->td_prio)
      return 0;
    else
      return turnstileq_sorted_forw(next);
  }
}

static int turnstileq_sorted_back(thread_t *td) {
  if (td == NULL)
    return 1;
  else {
    thread_t *prev = TAILQ_PREV(td, threadqueue, td_turnstileq);
    if (prev != NULL && prev->td_prio < td->td_prio)
      return 0;
    else
      return turnstileq_sorted_back(prev);
  }
}

static int turnstileq_sorted(thread_t *td) {
  return turnstileq_sorted_back(td) && turnstileq_sorted_forw(td);
}

static int test_turnstile_adjust(void) {
  for (int i = 0; i < T; i++) {
    assert(new_priorities[i] < RQ_PPQ);
  }

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

  // now all the threads should be blocked on the mutex

  // TODO could make these random instead
  for (int i = 0; i < T; i++) {
    WITH_SPINLOCK(threads[i]->td_spin) {
      sched_set_prio(threads[i], new_priorities[i]);
    }

    assert(turnstileq_sorted(threads[i]));
  }

  mtx_unlock(&ts_adj_mtx);

  for (int i = 0; i < T; i++)
    thread_join(threads[i]);

  return KTEST_SUCCESS;
}

KTEST_ADD(turnstile_adjust, test_turnstile_adjust, 0);
