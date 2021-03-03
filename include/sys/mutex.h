#ifndef _SYS_MUTEX_H_
#define _SYS_MUTEX_H_

#include <stdbool.h>
#include <sys/mimiker.h>
#include <sys/_lock.h>
#include <sys/lockdep.h>

typedef struct thread thread_t;

/*! \file mutex.h */

/*! \brief Basic synchronization primitive.
 *
 * \warning You must never access mutex fields directly outside of its
 * implementation!
 *
 * \note Mutex must be released by its owner!
 */
typedef struct mtx {
  lk_attr_t m_attr;          /*!< lock attributes */
  volatile unsigned m_count; /*!< counter for recursive mutexes */
  atomic_intptr_t m_owner;   /*!< stores address of the owner */

#if LOCKDEP
  lock_class_mapping_t m_lockmap;
#endif
} mtx_t;

/* Flags stored in lower 3 bits of m_owner. */
#define MTX_CONTESTED 1
#define MTX_FLAGMASK 7

#if LOCKDEP
#define MTX_INITIALIZER(lockname, recursive)                                   \
  (mtx_t) {                                                                    \
    .m_attr = (recursive) | LK_TYPE_BLOCK, .m_lockmap = {                      \
      .key = NULL,                                                             \
      .name = #lockname,                                                       \
      .lock_class = NULL                                                       \
    }                                                                          \
  }
#else
#define MTX_INITIALIZER(lockname, recursive)                                   \
  (mtx_t) {                                                                    \
    .m_attr = (recursive) | LK_TYPE_BLOCK                                      \
  }
#endif

/*! \brief Initializes mutex.
 *
 * \note Every mutex has to be initialized before it is used. */
void _mtx_init(mtx_t *m, lk_attr_t attr, const char *name,
               lock_class_key_t *key);

#define mtx_init(lock, attr)                                                   \
  do {                                                                         \
    static lock_class_key_t __key;                                             \
    _mtx_init(lock, attr, #lock, &__key);                                      \
  } while (0);

/*! \brief Makes mutex unusable for further locking.
 *
 * \todo Not implemented yet. */
#define mtx_destroy(m)

/*! \brief Check if calling thread is the owner of \a m. */
bool mtx_owned(mtx_t *m);

/*! \brief Fetch mutex owner.
 *
 * \note The function is used by some tests. */
static inline thread_t *mtx_owner(mtx_t *m) {
  return (thread_t *)(m->m_owner & ~MTX_FLAGMASK);
}

/*! \brief Locks sleep mutex (with custom \a waitpt) */
void _mtx_lock(mtx_t *m, const void *waitpt);

/*! \brief Locks sleep mutex.
 *
 * If mutex is already owned, then the thread is inserted into turnstile. */
static inline void mtx_lock(mtx_t *m) {
  _mtx_lock(m, __caller(0));
}

/*! \brief Unlocks sleep mutex */
void mtx_unlock(mtx_t *m);

/*! \brief Locks a pair of distinct mutexes belonging to the same class.
 *
 * The mutex with the lower address is locked first. */
void mtx_lock_pair(mtx_t *m1, mtx_t *m2);

/*! \brief Unlocks a pair of distinct mutexes belonging to the same class.
 *
 * The mutex with the higher address is unlocked first. */
void mtx_unlock_pair(mtx_t *m1, mtx_t *m2);

DEFINE_CLEANUP_FUNCTION(mtx_t *, mtx_unlock);

/*! \brief Locks sleep mutex and unlocks it when leaving current scope.
 *
 * You may safely leave the scope by using `break` or `return`.
 *
 * \warning Do not call `noreturn` functions before you leave the scope!
 */
#define SCOPED_MTX_LOCK(mtx_p)                                                 \
  SCOPED_STMT(mtx_t, mtx_lock, CLEANUP_FUNCTION(mtx_unlock), mtx_p)

/*! \brief Enter scope with sleep mutex locked.
 *
 * Unlocks the mutex when leaving the scope.
 *
 * \sa SCOPED_MTX_LOCK
 */
#define WITH_MTX_LOCK(mtx_p)                                                   \
  WITH_STMT(mtx_t, mtx_lock, CLEANUP_FUNCTION(mtx_unlock), mtx_p)

#endif /* !_SYS_MUTEX_H_ */
