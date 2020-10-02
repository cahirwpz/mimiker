#include <sys/mimiker.h>
#include <sys/ktest.h>
#include <sys/mutex.h>
#include <sys/runq.h>
#include <sys/sched.h>
#include <sys/thread.h>
#include <sys/sleepq.h>

#define T 5

/* Test of turnstile_adjust function.
 *
 * We block T threads on a mutex, then change their priorities via
 * sched_set_prio (which calls turnstile_adjust).
 * Test passes if threads are properly sorted on turnstile ts_blocked list. */

typedef TAILQ_HEAD(td_queue, thread) td_queue_t;

static mtx_t ts_adj_mtx = MTX_INITIALIZER(0);
static thread_t *threads[T];

static void set_prio(thread_t *td, prio_t prio) {
  WITH_SPIN_LOCK (td->td_lock)
    sched_set_prio(td, prio);
}

static bool td_is_blocked_on_mtx(thread_t *td, mtx_t *m) {
  assert(td_is_blocked(td));
  return td->td_wchan == m;
}

static void routine(void *_arg) {
  WITH_NO_PREEMPTION {
    sleepq_signal(thread_self());
    mtx_lock(&ts_adj_mtx);
  }

  mtx_unlock(&ts_adj_mtx);
}

static int lockq_sorted_forw(thread_t *td) {
  if (td == NULL)
    return 1;
  else {
    thread_t *next = TAILQ_NEXT(td, td_blockedq);
    if (next != NULL && prio_gt(next->td_prio, td->td_prio))
      return 0;
    else
      return lockq_sorted_forw(next);
  }
}

static int lockq_sorted_back(thread_t *td) {
  if (td == NULL)
    return 1;
  else {
    thread_t *prev = TAILQ_PREV(td, td_queue, td_blockedq);
    if (prev != NULL && prio_lt(prev->td_prio, td->td_prio))
      return 0;
    else
      return lockq_sorted_back(prev);
  }
}

static int turnstile_sorted(thread_t *td) {
  return lockq_sorted_back(td) && lockq_sorted_forw(td);
}

static int test_turnstile_adjust(void) {
  prio_t starting_priority = prio_kthread(90);
  prio_t new_priorities[T] = {prio_kthread(80), prio_kthread(50),
                              prio_kthread(170), prio_kthread(10),
                              prio_kthread(30)};

  for (int i = 0; i < T; i++) {
    char name[20];
    snprintf(name, sizeof(name), "test-turnstile-adjust-%d", i);
    threads[i] = thread_create(name, routine, NULL, starting_priority);
  }

  mtx_lock(&ts_adj_mtx);

  for (int i = 0; i < T; i++) {
    WITH_NO_PREEMPTION {
      sched_add(threads[i]);
      sleepq_wait(threads[i], "thread start");
    }
  }

  for (int i = 0; i < T; i++)
    assert(td_is_blocked_on_mtx(threads[i], &ts_adj_mtx));

  for (int i = 0; i < T; i++) {
    set_prio(threads[i], new_priorities[i]);
    assert(turnstile_sorted(threads[i]));
  }

  mtx_unlock(&ts_adj_mtx);

  for (int i = 0; i < T; i++)
    thread_join(threads[i]);

  return KTEST_SUCCESS;
}

KTEST_ADD(turnstile_adjust, test_turnstile_adjust, 0);
