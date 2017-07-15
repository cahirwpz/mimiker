#ifndef _SYS_SCHED_H_
#define _SYS_SCHED_H_

#include <common.h>

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

void sched_add(thread_t *td);
void sched_remove(thread_t *td);
void sched_clock(void);
void sched_switch(void);

noreturn void sched_run(void);

#endif /* !_SYS_SCHED_H_ */
