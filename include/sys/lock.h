#ifndef _SYS_LOCK_H_
#define _SYS_LOCK_H_

/*! \enum Kind of lock. */
typedef enum {
  /*!\var LK_BLOCK
   * \brief Type of blocking locks.
   *
   * When a thread tries to acquire a blocking lock that is owned by another
   * thread, it will block and switch out to another thread. */
  LK_BLOCK = 0x1,
  /*!\var LK_SPIN
   * \brief Type of spin locks.
   *
   * Interrupts are disabled upon acquiring a spin lock. */
  LK_SPIN = 0x2,
  /*!\var LK_SLEEP
   * \brief Type of sleeping locks.
   *
   * When a thread tries to acquire a sleeping lock that is owned by another
   * thread, it will go to sleep on a sleepqueue. */
  LK_SLEEP = 0x3,
  /*!\var LK_RECURSE
   * \brief Flag indicating a recursive lock.
   *
   * The lock may be acquired by the owner multiple times, and must
   * be released exactly as many times. */
  LK_RECURSE = 0x4
} lock_type_t;

/*!\brief Mask used to extract a lock's type (blocking/spin/...). */
#define LK_TYPE_MASK 0x3

typedef struct spin spin_t;
typedef struct mtx mtx_t;

#endif /* !_SYS_LOCK_H_ */
