#ifndef _SYS_TURNSTILE_H_
#define _SYS_TURNSTILE_H_

#include <sys/cdefs.h>
#include <sys/priority.h>

typedef struct mtx mtx_t;
typedef struct turnstile turnstile_t;

/*! \brief Called during kernel initialization. */
void init_turnstile(void);

void turnstile_lock(mtx_t *mtx);

void turnstile_unlock(mtx_t *mtx);

/*! \brief Allocates turnstile entry. */
turnstile_t *turnstile_alloc(void);

/*! \brief Deallocates turnstile entry. */
void turnstile_destroy(turnstile_t *ts);

/* Scheduler should call this after changing priority of a thread that is
 * blocked on some turnstile.
 *
 * It will re-sort list of blocked threads, on which `td` is and possibly
 * propagate `td`'s priority if it was increased.
 *
 * If `td` was lending priority and it was decreased, we don't change lent
 * priorities. Unlending would be problematic and the borrowing threads should
 * finish soon anyway.
 *
 * \note Requires td_spin acquired. */
void turnstile_adjust(thread_t *td, prio_t oldprio);

/* Block the current thread on given turnstile. This function will perform
 * context switch and release turnstile when woken up. */
void turnstile_wait(mtx_t *mtx, const void *waitpt);

/* Wakeup all threads waiting on given mutex and adjust the priority of the
 * current thread appropriately. */
void turnstile_broadcast(mtx_t *mtx);

#endif /* !_SYS_TURNSTILE_H_ */
