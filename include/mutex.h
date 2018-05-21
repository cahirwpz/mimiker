#ifndef _SYS_MUTEX_H_
#define _SYS_MUTEX_H_

#include <stdbool.h>
#include <common.h>

typedef struct thread thread_t;

/*! \file mutex.h */

/*! \brief Type of default non-recursive sleeping mutex.
 *
 * When a thread acquires sleeping mutex that is owned by another thread it will
 * be suspended and put on a sleep queue.
 */
#define MTX_DEF 0

/*! \brief Type of recursive sleeping mutex.
 *
 * The owner may lock it multiple times, but must release it as many times as
 * she acquired it.
 *
 * \sa MTX_DEF
 */
#define MTX_RECURSE 1

/*! \brief Basic synchronization primitive.
 *
 * \note Mutex must be released by its owner!
 */
typedef struct mtx {
  volatile thread_t *m_owner; /*!< stores address of the owner */
  volatile unsigned m_count;  /*!< counter for recursive mutexes */
  unsigned m_type;            /*!< type of mutex */
  const void *m_lockpt;       /*!< place where the lock was acquired */
} mtx_t;

#define MTX_INITIALIZER(type)                                                  \
  (mtx_t) {                                                                    \
    .m_owner = NULL, .m_count = 0, .m_type = type, .m_lockpt = NULL            \
  }

/*! \brief Initializes mutex.
 *
 * \note Every mutex has to be initialized before it is used. */
void mtx_init(mtx_t *m, unsigned type);

/*! \brief Makes mutex unusable for further locking.
 *
 * \todo Not implemented yet. */
#define mtx_destroy(m)

/*! \brief Check if calling thread is the owner of \a m. */
bool mtx_owned(mtx_t *m);

/*! \brief Locks the mutex (with custom \a waitpt) */
void _mtx_lock(mtx_t *m, const void *waitpt);

/*! \brief Locks the mutex.
 *
 * For sleeping mutexes, if mutex is already owned by someone,
 * then the thread is put onto sleep queue. */
static inline void mtx_lock(mtx_t *m) {
  _mtx_lock(m, __caller(0));
}

/*! \brief Unlocks the mutex. */
void mtx_unlock(mtx_t *m);

DEFINE_CLEANUP_FUNCTION(mtx_t *, mtx_unlock);

/*! \brief Mutex owner getter. */
static inline thread_t *mtx_owner(mtx_t *m) {
  return m->m_owner;
}

/*! \brief Acquire mutex and release it when leaving current scope.
 *
 * You may safely leave the scope by using `break` or `return`.
 *
 * \warning Do not call `noreturn` functions before you leave the scope!
 */
#define SCOPED_MTX_LOCK(mtx_p)                                                 \
  SCOPED_STMT(mtx_t, mtx_lock, CLEANUP_FUNCTION(mtx_unlock), mtx_p)

/*! \brief Enter scope with mutex acquired.
 *
 * Releases the mutex when leaving the scope.
 *
 * \sa SCOPED_MTX_LOCK
 */
#define WITH_MTX_LOCK(mtx_p)                                                   \
  WITH_STMT(mtx_t, mtx_lock, CLEANUP_FUNCTION(mtx_unlock), mtx_p)

#endif /* !_SYS_MUTEX_H_ */
