#ifndef _SYS_LOCK_H_
#define _SYS_LOCK_H_

/*! \enum Kind of lock. */
typedef enum {
  /*!\var LK_SLEEP
   * \brief Type of sleeping lock.
   *
   * When a thread acquires sleeping lock that is owned by another thread it
   * will be suspended and put on a sleep queue. */
  LK_SLEEP = 0,
  /*!\var LK_SPIN
   * \brief Type of spinning lock.
   *
   * When a thread acquires spinning lock interrupts will be disabled.
   */
  LK_SPIN = 1,
  /*!\var MTX_RECURSE
   * \brief Type of recursive lock.
   *
   * The owner may lock it multiple times, but must release it as many times as
   * she acquired it. */
  LK_RECURSE = 2,
} lock_type_t;

typedef struct spin spin_t;
typedef struct mtx mtx_t;

#endif /* !_SYS_LOCK_H_ */
