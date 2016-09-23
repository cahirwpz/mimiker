#ifndef __SCHED_H__
#define __SCHED_H__

#include <common.h>

typedef struct thread thread_t;

void sched_init();
void sched_add(thread_t *td);
void sched_remove(thread_t *td);
void sched_clock();
void sched_yield();
void sched_switch(thread_t *newtd);

noreturn void sched_run();

#endif // __SCHED_H__
