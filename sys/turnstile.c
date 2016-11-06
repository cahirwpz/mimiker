#include <stdc.h>
#include <sched.h>
#include <turnstile.h>
#include <sync.h>
#include <atomic.h>

void turnstile_init(turnstile_t *turnstile) {
  TAILQ_INIT(&turnstile->td_queue);
}

void turnstile_wait(turnstile_t *ts) {
  cs_enter();
  struct thread *td = thread_self();
  TAILQ_INSERT_TAIL(&ts->td_queue, td, td_lock);
  td->td_state = TDS_WAITING;
  cs_leave();
  sched_yield();
}

void turnstile_signal(turnstile_t *ts) {
  cs_enter();
  struct thread *newtd = TAILQ_FIRST(&ts->td_queue);
  if (newtd) {
    TAILQ_REMOVE(&ts->td_queue, newtd, td_lock);
    sched_add(newtd);
  }
  cs_leave();
}
