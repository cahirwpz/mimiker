#include <sys/klog.h>
#include <sys/mutex.h>
#include <sys/interrupt.h>
#include <sys/turnstile.h>
#include <sys/sched.h>
#include <sys/thread.h>

MTX_DEFINE(blocked_lock, MTX_SPIN);

bool mtx_owned(mtx_t *m) {
  return (mtx_owner(m) == thread_self());
}

void init_mtx(void) {
  mtx_init(&blocked_lock, MTX_SPIN);
  blocked_lock.m_owner = 0xdeadc0de;
}

void _mtx_init(mtx_t *m, intptr_t flags, const char *name,
               lock_class_key_t *key) {
  assert((flags & ~(MTX_SPIN | MTX_NODEBUG)) == 0);
  m->m_owner = flags;

#if LOCKDEP
  m->m_lockmap =
    (lock_class_mapping_t){.key = key, .name = name, .lock_class = NULL};
#endif
}

void _mtx_lock(mtx_t *m, const void *waitpt) {
  intptr_t flags = m->m_owner & (MTX_SPIN | MTX_NODEBUG);

  if (flags & MTX_SPIN)
    intr_disable();
  else if (__unlikely(intr_disabled()))
    panic("Cannot acquire sleep mutex in interrupt context!");

  if (__unlikely(mtx_owned(m)))
    panic("Attempt was made to re-acquire a mutex!");

#if LOCKDEP
  if (!(flags & MTX_NODEBUG))
    lockdep_acquire(&m->m_lockmap);
#endif

  thread_t *td = thread_self();

  for (;;) {
    intptr_t cur = flags;
    intptr_t new = (intptr_t)td | flags;

    /* Fast path: if lock has no owner then take ownership. */
    if (atomic_compare_exchange_strong(&m->m_owner, &cur, new))
      break;

    if (flags & MTX_SPIN)
      continue;

    turnstile_lock(m);
    cur = m->m_owner;

  retry_turnstile:

    if (cur == flags) {
      turnstile_unlock(m);
      continue;
    }

    if (cur & MTX_CONTESTED)
      goto block;

    new = cur | MTX_CONTESTED;
    if (!atomic_compare_exchange_strong(&m->m_owner, &cur, new))
      goto retry_turnstile;

  block:
    assert(mtx_owned(m));
    turnstile_wait(m, waitpt);
  }
}

void mtx_unlock(mtx_t *m) {
  intptr_t flags = m->m_owner & (MTX_SPIN | MTX_NODEBUG);

  assert(mtx_owned(m));

#if LOCKDEP
  if (!(flags & MTX_NODEBUG))
    lockdep_release(&m->m_lockmap);
#endif

  /* Fast path: if lock is not contested then drop ownership. */
  intptr_t cur = (intptr_t)thread_self() | flags;
  intptr_t new = flags;

  if (atomic_compare_exchange_strong(&m->m_owner, &cur, new))
    goto done;

  assert(!(flags & MTX_SPIN));

  /* Using broadcast instead of signal is faster according to
   * "The Design and Implementation of the FreeBSD Operating System",
   * 2nd edition, 4.3 Context Switching, page 138.
   *
   * The reasoning is that the awakened threads will often be scheduled
   * sequentially and only act on empty mutex on which operations are
   * cheaper. */
  turnstile_lock(m);
  uintptr_t owner = atomic_exchange(&m->m_owner, flags & MTX_NODEBUG);
  if (owner & MTX_CONTESTED)
    turnstile_broadcast(m);
  turnstile_unlock(m);

done:
  if (flags & MTX_SPIN)
    intr_enable();
}
