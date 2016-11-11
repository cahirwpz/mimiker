#include <sync.h>
#include <atomic.h>
#include <mutex.h>

#define MTX_UNOWNED 0

int mtx_owned(mtx_t *mtx) {
  return mtx->mtx_state != MTX_UNOWNED;
}

thread_t *mtx_owner(mtx_t *mtx) {
  return (thread_t*)mtx->mtx_state;
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
  assert(mtx_owner(mtx) != thread_self()); // No recursive mutexes for now
  while (!mtx_try_to_lock(mtx)) {
    /* 1. mutex can be released here */
    cs_enter();
    if(mtx->mtx_state == MTX_UNOWNED) /* Check if mutex was released at '1'*/
    {
        cs_leave();
        continue;
    }
    /* Mutex cannot be released here */
    turnstile_wait(&mtx->turnstile);
  }
}

void mtx_unlock(mtx_t *mtx) {
  atomic_store(&mtx->mtx_state, MTX_UNOWNED);
  turnstile_signal(&mtx->turnstile);
}
