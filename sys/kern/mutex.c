#include <sys/mimiker.h>
#include <sys/mutex.h>
#include <sys/turnstile.h>
#include <sys/sched.h>
#include <sys/thread.h>

bool mtx_owned(mtx_t *m) {
  return (mtx_owner(m) == thread_self());
}

void mtx_init(mtx_t *m, lk_attr_t la) {
  /* The caller must not attempt to set the lock's type, only flags. */
  assert((la & LK_TYPE_MASK) == 0);
  m->m_owner = 0;
  m->m_count = 0;
  m->m_attr = la | LK_TYPE_BLOCK;
}

void _mtx_lock(mtx_t *m, const void *waitpt) {
  if (mtx_owned(m)) {
    if (!lk_recursive_p(m))
      panic("Sleeping mutex %p is not recursive!", m);
    m->m_count++;
    return;
  }

  thread_t *td = thread_self();

  WITH_NO_PREEMPTION {
    while (m->m_owner) {
      turnstile_t *ts = turnstile_take(m);

      /* we're the first thread to block, so lock is now being contested */
      if (ts == td->td_turnstile)
        m->m_owner |= MTX_CONTESTED;

      turnstile_wait(ts, mtx_owner(m), waitpt);
    }

    m->m_owner = (intptr_t)td;
  }
}

void mtx_unlock(mtx_t *m) {
  assert(mtx_owned(m));

  if (m->m_count > 0) {
    assert(lk_recursive_p(m));
    m->m_count--;
    return;
  }

  WITH_NO_PREEMPTION {
    uintptr_t owner = m->m_owner;
    m->m_owner = 0;

    /* Using broadcast instead of signal is faster according to
     * "The Design and Implementation of the FreeBSD Operating System",
     * 2nd edition, 4.3 Context Switching, page 138.
     *
     * The reasoning is that the awakened threads will often be scheduled
     * sequentially and only act on empty mutex on which operations are
     * cheaper. */
    if (owner & MTX_CONTESTED)
      turnstile_broadcast(m);
  }
}
