#ifndef __SCHED_H__
#define __SCHED_H__

#include <thread.h>

void sched_run(size_t quantum);
void sched_init();
void sched_add(thread_t *td);
void sched_yield();
void sched_resume();
void sched_exit();

#endif // __SCHED_H__
