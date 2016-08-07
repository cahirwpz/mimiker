#ifndef __SCHED_H__
#define __SCHED_H__

#include <thread.h>

void sched_run();

void sched_init();

void sched_add(thread_t *td);

void sched_preempt();

void sched_yield();

void sched_switch();

#endif // __SCHED_H__
