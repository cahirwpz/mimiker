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
  LK_TYPE_BLOCK = 0x1,
  /*!\var LK_TYPE_SPIN
   * \brief Type of spin locks.
   *
   * Interrupts are disabled upon acquiring a spin lock. */
  LK_TYPE_SPIN = 0x2,
  /*!\var LK_TYPE_SLEEP
   * \brief Type of sleeping locks.
   *
   * When a thread tries to acquire a sleeping lock that is owned by another
   * thread, it will go to sleep on a sleepqueue. */
  LK_TYPE_SLEEP = 0x3,
  /*!\var LK_RECURSE
   * \brief Flag indicating a recursive lock.
   *
   * The lock may be acquired by the owner multiple times, and must
   * be released exactly as many times. */
  LK_RECURSE = 0x4
} lk_attr_t;

typedef struct spin spin_t;
typedef struct mtx mtx_t;

/*!\brief Mask used to extract a lock's type (blocking/spin/...). */
#define LK_TYPE_MASK 0x3

/* !\brief Get lock type from attributes */
#define lk_attr_type(attrs) ((attrs)&LK_TYPE_MASK)

/* !\brief Getters for attributes' type */
static inline bool lk_attr_blocking(lk_attr_t attrs) {
  return lk_attr_type(attrs) == LK_TYPE_BLOCK;
}

static inline bool lk_attr_spinning(lk_attr_t attrs) {
  return lk_attr_type(attrs) == LK_TYPE_SPIN;
}

static inline bool lk_attr_sleeping(lk_attr_t attrs) {
  return lk_attr_type(attrs) == LK_TYPE_SLEEP;
}

/* !\brief Getters for attributes' flags */
static inline bool lk_attr_recursive(lk_attr_t attrs) {
  return attrs & LK_RECURSE;
}

#endif /* !_SYS_LOCK_H_ */
