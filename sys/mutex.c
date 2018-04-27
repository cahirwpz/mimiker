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
      turnstile_t *ts = turnstile_acquire(m);
      /* In case of SMP we would have to check now whether some other
       * processor released the mutex while we were spinning for turnstile's
       * spinlock. */
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
    turnstile_t *ts = turnstile_lookup(m);
    if (ts != NULL) {
      /* Using broadcast instead of signal is faster according to
       * "The Design and Implementation of the FreeBSD Operating System
       * (2nd Edition)", 4.3 Context Switching, page 138.
       *
       * The reasoning is that the awakened threads will often be scheduled
       * sequentially and only act on empty mutex on which operations are
       * cheaper. */
      turnstile_broadcast(ts);
    } else
      /* The lock wasn't contested, nothing to do with turnstiles.
       * Just release spinlock acquired in turnstile_lookup. */
      turnstile_chain_unlock(m);
  }
}
