#include <atomic.h>
#include <mutex.h>

#define MTX_UNOWNED 0

int mtx_owned(mtx_t *mtx) {
  return mtx->mtx_state == (uint32_t)thread_self();
}

static int mtx_try_to_lock(mtx_t *mtx) {
  return atomic_cmp_exchange(&mtx->mtx_state, MTX_UNOWNED,
                             (uint32_t)thread_self());
}

void mtx_init(mtx_t *mtx) {
  mtx->mtx_state = MTX_UNOWNED;
  turnstile_init(&mtx->turnstile);
}

void mtx_lock(mtx_t *mtx) {
  assert(!mtx_owned(mtx)); // No recursive mutexes for now
  while (!mtx_try_to_lock(mtx)) {
    turnstile_wait(&mtx->turnstile);
  }
}

void mtx_unlock(mtx_t *mtx) {
  atomic_store(&mtx->mtx_state, MTX_UNOWNED);
  turnstile_signal(&mtx->turnstile);
}
