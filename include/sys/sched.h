#ifndef _SYS_SCHED_H_
#define _SYS_SCHED_H_

#ifdef _KERNEL

#include <sys/cdefs.h>
#include <sys/thread.h>

/*! \brief Called during kernel initialization. */
void init_sched(void);

/*! \brief Disables preemption.
 *
 * Prevents current thread from switching out on return from interrupt,
 * exception or trap handler. It does not disable interrupts!
 *
 * \sa on_exc_leave
 *
 * Calls to \fn preempt_disable can nest, you must use the same number of calls
 * to \fn preempt_enable to actually enable preemption.
 */
void preempt_disable(void);

/*! \brief Enables preemption. */
void preempt_enable(void);

/*! \brief Checks if current thread cannot be preempted now. */
bool preempt_disabled(void);

/* Two following functions are workaround to make preemption disabling work with
 * scoped and with statement. */
static inline void __preempt_disable(void *data) {
  preempt_disable();
}

static inline void __preempt_enable(void *data) {
  preempt_enable();
}

#define SCOPED_NO_PREEMPTION()                                                 \
  SCOPED_STMT(void, __preempt_disable, __preempt_enable, NULL)

#define WITH_NO_PREEMPTION                                                     \
  WITH_STMT(void, __preempt_disable, __preempt_enable, NULL)

/*! \brief Add new thread to the scheduler.
 *
 * The thread will be set runnable. */
void sched_add(thread_t *td);

/*! \brief Wake up sleeping thread.
 *
 * \param reason is a value that will be returned by sched_switch.
 * \note Must be called with \a td_spin acquired!
 */
void sched_wakeup(thread_t *td, long reason);

/*! \brief Lend a priority to a thread.
 *
 * The new priority must be higher than the old one.
 * Used as a part of a mechanism for avoiding priority inversion.
 *
 * \note Must be called with \a td_spin acquired!
 */
void sched_lend_prio(thread_t *td, prio_t prio);

/*! \brief Remove lent priority while offering a new priority to lend.
 *
 * \a prio priority will be lent if it's higher than thread's \a td_base_prio.
 * Used as a part of a mechanism for avoiding priority inversion.
 *
 * \note Must be called with \a td_spin acquired!
 */
void sched_unlend_prio(thread_t *td, prio_t prio);

/*! \brief Set thread's \a td_base_prio and \a td_prio to \a prio.
 *
 * Base priority \a td_base_prio is changed unconditionally.
 * Active priority \a td_prio is changed on condition that we are not lowering
 * priority of a thread that borrows priority via \a sched_lend_prio.
 *
 * \note Must be called with \a td_spin acquired!
 */
void sched_set_prio(thread_t *td, prio_t prio);

/*! \brief Takes care of run-time accounting for current thread.
 *
 * \note Must be called from interrupt context.
 */
void sched_clock(void);

/*! \brief Switch out to another thread.
 *
 * Before you call this function, set thread's state to requested value, which
 * must be different from TDS_RUNNING. \a sched_switch will modify thread's
 * field to reflect the change in state.
 *
 * \returns a value that was passed to sched_wakeup
 * \note Must be called with \a td_lock acquired, which will be unlocked after
 *       procedure returns!
 */
long sched_switch(void);

/*! \brief Switch out to another thread if you shouldn't be running anymore.
 *
 * This function will switch if your time slice expired or a thread with higher
 * priority has been added. It doesn't actually perform that check, it only
 * looks at TDF_NEEDSWITCH flag.
 *
 * \note Returns immediately if interrupts or preemption are disabled.
 */
void sched_maybe_preempt(void);

/*! \brief Turns calling thread into idle thread. */
__noreturn void sched_run(void);

#endif /* _KERNEL */

#endif /* !_SYS_SCHED_H_ */
