#ifndef __WORKER_H__
#define __WORKER_H__

#include <taskqueue.h>

taskqueue_t *workqueue;

void worker_init();
void worker_invoke(void *p);

#endif
