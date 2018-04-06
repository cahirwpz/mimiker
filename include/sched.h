#ifndef _SYS_SCHED_H_
#define _SYS_SCHED_H_

#include <common.h>
#include <thread.h>

typedef struct thread thread_t;

/*! \brief Disables preemption.
 *
 * Prevents scheduler from switching out current thread. Does not disable
 * interrupts.
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
 * \note Must be called with preemption disabled!
 */
void sched_wakeup(thread_t *td);

void sched_lend_prio(thread_t *td, td_prio_t prio);

void sched_unlend_prio(thread_t *td, td_prio_t prio);

/*! \brief Takes care of run-time accounting for current thread. */
void sched_clock(void);

/*! \brief Switch out to another thread.
 *
 * Before you call this function, set thread's state to requested value, which
 * must be different from TDS_RUNNING. \a sched_switch will modify thread's
 * field to reflect the change in state.
 *
 * \note Must be called with preemption disabled!
 */
void sched_switch(void);

/*! \brief Turns calling thread into idle thread. */
noreturn void sched_run(void);

#endif /* !_SYS_SCHED_H_ */
