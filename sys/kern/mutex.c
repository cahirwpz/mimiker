#include <sys/klog.h>
#include <sys/mutex.h>
#include <sys/interrupt.h>
#include <sys/turnstile.h>
#include <sys/sched.h>
#include <sys/thread.h>

bool mtx_owned(mtx_t *m) {
  return (mtx_owner(m) == thread_self());
}

void _mtx_init(mtx_t *m, intptr_t flags, const char *name,
               lock_class_key_t *key) {
  assert((flags & ~MTX_SPIN) == 0);
  m->m_owner = flags;

#if LOCKDEP
  m->m_lockmap =
    (lock_class_mapping_t){.key = key, .name = name, .lock_class = NULL};
#endif
}

void _mtx_lock(mtx_t *m, const void *waitpt) {
  intptr_t spin = m->m_owner & MTX_SPIN;

  if (spin) {
    intr_disable();
  } else {
    if (__unlikely(intr_disabled()))
      panic("Cannot acquire sleep mutex in interrupt context!");
  }

  if (__unlikely(mtx_owned(m)))
    panic("Attempt was made to re-acquire non-recursive mutex!");

#if LOCKDEP
  if (!spin)
    lockdep_acquire(&m->m_lockmap);
#endif

  thread_t *td = thread_self();

  for (;;) {
    intptr_t expected = spin;
    intptr_t value = (intptr_t)td | spin;

    /* Fast path: if lock has no owner then take ownership. */
    if (atomic_compare_exchange_strong(&m->m_owner, &expected, value))
      break;

    if (spin)
      continue;

    WITH_NO_PREEMPTION {
      /* TODO(cahir) turnstile_take / turnstile_give doesn't make much sense
       * until tc_lock is thrown into the equation. */
      turnstile_t *ts = turnstile_take(m);

      /* Between atomic cas and turnstile_take there's a small window when
       * preemption can take place. This can result in mutex being released. */
      if (m->m_owner) {
        /* we're the first thread to block, so lock is now being contested */
        if (ts == td->td_turnstile)
          m->m_owner |= MTX_CONTESTED;

        turnstile_wait(ts, mtx_owner(m), waitpt);
      } else {
        turnstile_give(ts);
      }
    }
  }
}

void mtx_unlock(mtx_t *m) {
  intptr_t spin = m->m_owner & MTX_SPIN;

  assert(mtx_owned(m));

#if LOCKDEP
  if (!spin)
    lockdep_release(&m->m_lockmap);
#endif

  /* Fast path: if lock is not contested then drop ownership. */
  intptr_t expected = (intptr_t)thread_self() | spin;
  intptr_t value = spin;

  if (atomic_compare_exchange_strong(&m->m_owner, &expected, value))
    goto done;

  /* Using broadcast instead of signal is faster according to
   * "The Design and Implementation of the FreeBSD Operating System",
   * 2nd edition, 4.3 Context Switching, page 138.
   *
   * The reasoning is that the awakened threads will often be scheduled
   * sequentially and only act on empty mutex on which operations are
   * cheaper. */
  WITH_NO_PREEMPTION {
    uintptr_t owner = atomic_exchange(&m->m_owner, 0);
    if (owner & MTX_CONTESTED)
      turnstile_broadcast(m);
  }

done:
  if (spin)
    intr_enable();
}
