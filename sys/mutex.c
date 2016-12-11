#include <sync.h>
#include <atomic.h>
#include <mutex.h>

#define MTX_UNOWNED 0

bool mtx_owned(mtx_t *mtx) {
  return (mtx->mtx_state == (uint32_t)thread_self());
}

static bool mtx_locked(mtx_t *mtx) {
  return (mtx->mtx_state != MTX_UNOWNED);
}

static int mtx_try_lock(mtx_t *mtx) {
  return atomic_cmp_exchange(&mtx->mtx_state, MTX_UNOWNED,
                             (uint32_t)thread_self());
}

void mtx_init(mtx_t *mtx) {
  mtx->mtx_state = MTX_UNOWNED;
  turnstile_init(&mtx->turnstile);
}

void mtx_lock(mtx_t *mtx) {
  /* TODO: Implement recursive mutexes */
  assert(!mtx_owned(mtx));

  while (!mtx_try_lock(mtx)) {
    cs_enter();
    /* Check if the mutex got unlocked since a call to mtx_try_lock */
    if (mtx->mtx_state == MTX_UNOWNED) {
      cs_leave();
      continue;
    }
    assert(mtx_locked(mtx));
    turnstile_wait(&mtx->turnstile);
    cs_leave();
  }
}

void mtx_unlock(mtx_t *mtx) {
  cs_enter();
  mtx->mtx_state = MTX_UNOWNED;
  turnstile_signal(&mtx->turnstile);
  cs_leave();
}
