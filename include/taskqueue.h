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

typedef STAILQ_HEAD(, task) tq_list_t;
typedef struct taskqueue {
  mtx_t tq_mutex;
  condvar_t
    tq_nonempty; /* worker waits on this cv for tq_list to become non empty */
  tq_list_t tq_list;
  thread_t *tq_worker;
} taskqueue_t;

/* Creates a new taskqueue. */
void taskqueue_init(taskqueue_t* tq);
/* Appends a task onto a taskqueue. The task will be executed later by the
   taskqueue worker thread.  */
void taskqueue_add(taskqueue_t *tq, task_t *task);
/* Do not use this function. It is periodically called by the task queue worker
   thread. The only reason it is exposed is that some tests need to wait until
   the task queue is empty, and they exploit taskqueue_run to do that. */
void taskqueue_run(taskqueue_t *tq);
void task_init(task_t* task, void (*func)(void *), void *arg);

extern taskqueue_t workqueue;
/* Initializes the workqueue subsystem. */
void workqueue_init();

#endif
