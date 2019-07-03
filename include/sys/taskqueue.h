#ifndef _SYS_TASKQUEUE_H_
#define _SYS_TASKQUEUE_H_

#include <sys/queue.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

typedef struct task {
  STAILQ_ENTRY(task) t_link;
  void (*t_func)(void *); /* callback function */
  void *t_arg;            /* callback argument */
} task_t;

#define TASK_INIT(func, arg)                                                   \
  (task_t) {                                                                   \
    .t_func = (func), .t_arg = (void *)(arg)                                   \
  }

typedef STAILQ_HEAD(, task) task_list_t;

typedef struct taskqueue {
  mtx_t tq_mutex;
  /* worker waits on this cv for tq_list to become non empty */
  condvar_t tq_nonempty;
  task_list_t tq_list;
} taskqueue_t;

/* Creates a new taskqueue. */
void taskqueue_init(taskqueue_t *tq);
/* Destroys a taskqueue. */
void taskqueue_destroy(taskqueue_t *tq);
/* Appends a task onto a taskqueue. The task will be executed later in a thread
 * context. */
void taskqueue_add(taskqueue_t *tq, task_t *task);
/* Executes all task in the queue without holding a lock, i.e. new tasks may
 * arrive while old ones are processed. */
void taskqueue_run(taskqueue_t *tq);

#endif /* !_SYS_TASKQUEUE_H_ */
