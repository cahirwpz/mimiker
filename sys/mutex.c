#include <mutex.h>
#include <turnstile.h>
#include <interrupt.h>
#include <sched.h>
#include <thread.h>

bool mtx_owned(mtx_t *m) {
  return (m->m_owner == thread_self());
}

void mtx_init(mtx_t *m, unsigned type) {
  m->m_owner = NULL;
  m->m_count = 0;
  m->m_type = type;
}

void _mtx_lock(mtx_t *m, const void *waitpt) {
  if (mtx_owned(m)) {
    assert(m->m_type == MTX_RECURSE);
    m->m_count++;
    return;
  }

  WITH_NO_PREEMPTION {
    while (m->m_owner != NULL) {
      turnstile_t *ts = turnstile_trywait(m);
      // if we had SMP then something could happen while
      // we were spinning on tc_lock
      turnstile_wait(ts, (thread_t *)m->m_owner);
    }
    m->m_owner = thread_self();
    m->m_lockpt = waitpt;
  }
}

void mtx_unlock(mtx_t *m) {
  assert(mtx_owned(m));

  if (m->m_count > 0) {
    assert(m->m_type == MTX_RECURSE);
    m->m_count--;
    return;
  }

  WITH_NO_PREEMPTION {
    m->m_owner = NULL;
    m->m_lockpt = NULL;
    turnstile_chain_lock(m);
    turnstile_t *ts = turnstile_lookup(m);
    turnstile_chain_unlock(m);
    if (ts != NULL)
      // TODO signal instead of broadcast (not implemented yet)
      turnstile_broadcast(ts);
  }
}
