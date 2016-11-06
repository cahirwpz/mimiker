#include <stdc.h>
#include <sched.h>
#include <turnstile.h>
#include <sync.h>
#include <atomic.h>

void turnstile_wait(turnstile_t *ts) {
  cs_enter();
  struct thread *td = thread_self();
  TAILQ_INSERT_TAIL(&ts->td_queue, td, td_lock);
  td->td_state = TDS_WAITING;
  cs_leave();
  sched_yield();
}

void turnstile_dump(turnstile_t *ts) {
  thread_t *it;
  kprintf("Blocked on turnstile: \n");
  TAILQ_FOREACH (it, &ts->td_queue, td_lock) {
    kprintf("%s\n", it->td_name);
    if (TAILQ_NEXT(it, td_lock) == it)
      panic("Broken list!");
  }
  kprintf("\n");
}

void mutex_dump(mtx_sleep_t *mtx) {
  turnstile_dump(&mtx->turnstile);
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

int mtx_owned(mtx_sleep_t *mtx) {
  return mtx->mtx_state == (uint32_t)thread_self();
}

int mtx_sleep_try_to_lock(mtx_sleep_t *mtx) {
  return atomic_cmp_exchange(&mtx->mtx_state, MTX_UNOWNED,
                             (uint32_t)thread_self());
}

void mtx_sleep_init(mtx_sleep_t *mtx) {
  mtx->mtx_state = MTX_UNOWNED;
  turnstile_init(&mtx->turnstile);
}

void turnstile_init(turnstile_t *turnstile) {
  TAILQ_INIT(&turnstile->td_queue);
}

void mtx_sleep_lock(mtx_sleep_t *mtx) {
  assert(!mtx_owned(mtx)); // No recursive mutexes for now
  while (!mtx_sleep_try_to_lock(mtx)) {
    turnstile_wait(&mtx->turnstile);
  }
}

void mtx_sleep_unlock(mtx_sleep_t *mtx) {
  atomic_store(&mtx->mtx_state, MTX_UNOWNED);
  turnstile_signal(&mtx->turnstile);
}
