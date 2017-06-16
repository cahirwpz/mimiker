#ifndef _SYS_SCHED_H_
#define _SYS_SCHED_H_

#include <common.h>

typedef struct thread thread_t;

void sched_add(thread_t *td);
void sched_remove(thread_t *td);
void sched_clock(void);
void sched_yield(void);
void sched_switch(thread_t *newtd);

noreturn void sched_run(void);

#endif /* _SYS_SCHED_H_ */
