#ifndef _SYS_LOCK_H_
#define _SYS_LOCK_H_

#include <sys/_lock.h>
#include <sys/mutex.h>
#include <sys/spinlock.h>

static inline void lk_acquire(lock_t l, const void *waitpt) {
  if (lk_spin_p(l))
    _spin_lock(l.spin, waitpt);
  else
    _mtx_lock(l.mtx, waitpt);
}

static inline void lk_release(lock_t l) {
  if (lk_spin_p(l))
    spin_unlock(l.spin);
  else
    mtx_unlock(l.mtx);
}

#endif /* !_SYS_LOCK_H_ */
