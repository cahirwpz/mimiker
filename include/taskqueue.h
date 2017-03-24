#ifndef __SYS_TASKQUEUE_H__
#define __SYS_TASKQUEUE_H__

#include <queue.h>
#include <mutex.h>
#include <condvar.h>

typedef struct task {
  STAILQ_ENTRY(task) tq_link;
  void *arg;            /* function argument */
  void (*func)(void *); /* function pointer */
} task_t;

typedef struct taskqueue {
  mtx_t tq_mutex;
  condvar_t tq_nonempty; /* worker waits on this cv for tq_list to become non empty */
  STAILQ_HEAD(, task) tq_list;
  thread_t *tq_worker;
} taskqueue_t;

void taskqueue_init();
taskqueue_t *taskqueue_create();
void taskqueue_add(taskqueue_t *tq, task_t *task);
void taskqueue_run(taskqueue_t *tq);
task_t *task_create(void (*func)(void *), void *arg);

#endif
