#ifndef _SYS_LOCK_H_
#define _SYS_LOCK_H_

/* Lock flags */
typedef int lock_type_t;

/*!\brief Type of blocking locks.
 *
 * When a thread tries to acquire a blocking lock that is owned by another
 * thread, it will block and switch out to another thread. */
#define LK_BLOCKING 0x1
/*!\brief Type of spin locks.
 *
 * Interrupts are disabled upon acquiring a spin lock. */
#define LK_SPIN 0x2
/*!\brief Mask used to extract a lock's type (blocking/spin). */
#define LK_TYPE 0x3
/*!\brief Flag indicating a recursive lock.
 *
 * The lock may be acquired by the owner multiple times, and must
 * be released exactly as many times. */
#define LK_RECURSE 0x4

typedef struct spin spin_t;
typedef struct mtx mtx_t;

#endif /* !_SYS_LOCK_H_ */
