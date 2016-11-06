#include <atomic.h>
#include <mutex.h>

int mtx_owned(mtx_sleep_t *mtx) {
  return mtx->mtx_state == (uint32_t)thread_self();
}

static int mtx_sleep_try_to_lock(mtx_sleep_t *mtx) {
  return atomic_cmp_exchange(&mtx->mtx_state, MTX_UNOWNED,
                             (uint32_t)thread_self());
}

void mtx_sleep_init(mtx_sleep_t *mtx) {
  mtx->mtx_state = MTX_UNOWNED;
  turnstile_init(&mtx->turnstile);
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
