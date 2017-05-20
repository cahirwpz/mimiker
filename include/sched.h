#ifndef _SYS_SCHED_H_
#define _SYS_SCHED_H_

#include <common.h>

typedef struct thread thread_t;

void sched_add(thread_t *td);
void sched_remove(thread_t *td);
void sched_clock();
void sched_yield();
void sched_switch(thread_t *newtd);

noreturn void sched_run();

#endif /* _SYS_SCHED_H_ */
