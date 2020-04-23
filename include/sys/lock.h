#ifndef _SYS_LOCK_H_
#define _SYS_LOCK_H_

#include <stdbool.h>

/*!\brief Lock attributes.
 *
 * Non-mutually-exclusive members may be bitwise-ORed together.
 * \note Members that have any set bits in common with #LK_TYPE_MASK
 * are mutually exclusive and denote the lock's type. */
typedef enum {
  /*!\var LK_TYPE_BLOCK
   * \brief Type of blocking locks.
   *
   * When a thread tries to acquire a blocking lock that is owned by another
   * thread, it will block and switch out to another thread. */
  LK_TYPE_BLOCK = 1,
  /*!\var LK_TYPE_SPIN
   * \brief Type of spin locks.
   *
   * Interrupts are disabled upon acquiring a spin lock. */
  LK_TYPE_SPIN = 2,
  /*!\var LK_TYPE_SLEEP
   * \brief Type of sleeping locks.
   *
   * When a thread tries to acquire a sleeping lock that is owned by another
   * thread, it will go to sleep on a sleepqueue. */
  LK_TYPE_SLEEP = 3,
  /*!\var LK_RECURSIVE
   * \brief Flag indicating a recursive lock.
   *
   * The lock may be acquired by the owner multiple times, and must
   * be released exactly as many times. */
  LK_RECURSIVE = 4
} lk_attr_t;

typedef struct spin spin_t;
typedef struct mtx mtx_t;

/* Union type of locks that may be passed to `cv_wait` */
typedef union lock {
  lk_attr_t *attr; /*! `mtx_t` and `spin_t` must begin with `lk_attr_t` */
  mtx_t *mtx;      /*!< sleep mutex to use with `cv_wait` */
  spin_t *spin;    /*!< spin lock to use with `cv_wait`*/
} __transparent_union lock_t;

/*!\brief Mask used to extract a lock's type (blocking/spin/...). */
#define LK_TYPE_MASK 0x3

/* !\brief Get lock type from attributes */
#define lk_attr(l) (*(l).attr)
#define lk_type(l) (lk_attr(l) & LK_TYPE_MASK)

/* !\brief Predicates checking type of a lock */
static inline bool lk_spin_p(lock_t l) {
  return lk_type(l) == LK_TYPE_SPIN;
}

static inline bool lk_block_p(lock_t l) {
  return lk_type(l) == LK_TYPE_BLOCK;
}

static inline bool lk_sleep_p(lock_t l) {
  return lk_type(l) == LK_TYPE_SLEEP;
}

/* !\brief Predicates checking flags of a lock */
static inline bool lk_recursive_p(lock_t l) {
  return lk_attr(l) & LK_RECURSIVE;
}

#endif /* !_SYS_LOCK_H_ */
