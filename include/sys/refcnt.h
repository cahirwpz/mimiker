#ifndef _SYS_REFCNT_H_
#define _SYS_REFCNT_H_

#include <stdbool.h>
#include <stdatomic.h>
#include <sys/klog.h>
#include <limits.h>

typedef atomic_uint refcnt_t;

/*! \brief Atomically increase reference counter. */
static inline void refcnt_acquire(refcnt_t *refcnt_p) {
  unsigned old = atomic_fetch_add(refcnt_p, 1);
  if (old == UINT_MAX)
    panic("Reference count %p overflowed!", refcnt_p);
}

/*! \brief Atomically decrease reference counter.
 *
 * \returns true if reference counter reached value of 0 */
static inline bool refcnt_release(refcnt_t *refcnt_p) {
  unsigned old = atomic_fetch_sub(refcnt_p, 1);
  if (old == 0)
    panic("Reference counter %p dropped below zero!", refcnt_p);
  return old == 1;
}

#endif /* !_SYS_REFCNT_H_ */
