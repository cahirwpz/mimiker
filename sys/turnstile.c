#include <sync.h>
#include <queue.h>
#include <sched.h>
#include <turnstile.h>

void turnstile_init(turnstile_t *ts) {
  TAILQ_INIT(&ts->td_queue);
}

void turnstile_wait(turnstile_t *ts) {
  thread_t *td = thread_self();
  TAILQ_INSERT_TAIL(&ts->td_queue, td, td_lock);
  td->td_state = TDS_WAITING;
  sched_yield();
  cs_leave();
}

void turnstile_signal(turnstile_t *ts) {
  cs_enter();
  thread_t *newtd = TAILQ_FIRST(&ts->td_queue);
  if (newtd) {
    TAILQ_REMOVE(&ts->td_queue, newtd, td_lock);
    sched_add(newtd);
  }
  cs_leave();
}
