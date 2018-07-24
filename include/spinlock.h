#ifndef _SYS_SPINLOCK_H_
#define _SYS_SPINLOCK_H_

#include <stdbool.h>
#include <common.h>

typedef struct thread thread_t;

/*! \file spinlock.h */

/*! \brief Special purpose synchronization primitive.
 *
 * Spin lock should be used to synchronize kernel code wrt. interrupt contexts.
 *
 * Acquiring spin lock enters critical section, thus disables interrupts and
 * thread preemption.
 *
 * \attention It is forbidden to change context while holding spin lock!
 * \todo How to enforce the condition given above?
 *
 * \note Spin lock must be released by its owner!
 * \todo During transition from single-core to multi-core architecture spin lock
 *       will be used for interprocessor synchronization as well.
 */
typedef struct spinlock {
  volatile thread_t *s_owner; /*!< stores address of the owner */
  volatile unsigned s_count;  /*!< counter for recursive spinlock */
  const void *s_lockpt;       /*!< place where the lock was acquired */
} spinlock_t;

#define SPINLOCK_INITIALIZER()                                                 \
  (spinlock_t) {                                                               \
    .s_owner = NULL, .s_lockpt = NULL                                          \
  }

/*! \brief Initializes spin lock.
 *
 * \note Every spin lock has to be initialized before it is used. */
void spin_init(spinlock_t *s);

/*! \brief Makes spin lock unusable for further locking.
 *
 * \todo Not implemented yet. */
#define spin_destroy(m)

/*! \brief Check if calling thread is the owner of \a s. */
bool spin_owned(spinlock_t *s);

/*! \brief Acquires the spin lock (with custom \a waitpt) */
void _spin_acquire(spinlock_t *s, const void *waitpt);

/*! \brief Acquire the spin lock.
 *
 * \note On single core architecture it impossible to block on spinlock.
 */
static inline void spin_acquire(spinlock_t *s) {
  _spin_acquire(s, __caller(0));
}

/*! \brief Release the spin lock. */
void spin_release(spinlock_t *s);

DEFINE_CLEANUP_FUNCTION(spinlock_t *, spin_release);

/*! \brief Acquire spin lock and release it when leaving current scope.
 *
 * You may safely leave the scope by using `break` or `return`.
 *
 * \warning Do not call `noreturn` functions before you leave the scope!
 */
#define SCOPED_SPINLOCK(spin_p)                                                \
  SCOPED_STMT(spinlock_t, spin_acquire, CLEANUP_FUNCTION(spin_release), spin_p)

/*! \brief Enter scope with spin lock acquired.
 *
 * Releases the spin lock when leaving the scope.
 *
 * \sa SCOPED_SPINLOCK
 */
#define WITH_SPINLOCK(spin_p)                                                  \
  WITH_STMT(spinlock_t, spin_acquire, CLEANUP_FUNCTION(spin_release), spin_p)

#endif /* !_SYS_SPINLOCK_H_ */
