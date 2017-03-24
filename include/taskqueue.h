#ifndef __SYS_TASKQUEUE_H__
#define __SYS_TASKQUEUE_H__

#include <queue.h>

typedef struct task {
  STAILQ_ENTRY(task) tq_link;
  void *arg;            /* function argument */
  void (*func)(void *); /* function pointer */
} task_t;

typedef struct tq_head taskqueue_t;
STAILQ_HEAD(tq_head, task);

void taskqueue_init();
taskqueue_t *taskqueue_create();
void taskqueue_add(taskqueue_t *tq, task_t *task);
void taskqueue_run(taskqueue_t *tq);
task_t *task_create(void (*func)(void *), void *arg);

#endif
