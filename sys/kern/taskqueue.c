#include <sys/klog.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/taskqueue.h>

void taskqueue_init(taskqueue_t *tq) {
  STAILQ_INIT(&tq->tq_list);
  mtx_init(&tq->tq_mutex, LK_RECURSIVE);
  cv_init(&tq->tq_nonempty, "taskqueue nonempty");
}

void taskqueue_destroy(taskqueue_t *tq) {
  assert(STAILQ_EMPTY(&tq->tq_list));
  mtx_destroy(tq->tq_mutex);
  cv_destroy(tq->tq_nonempty);
}

void taskqueue_add(taskqueue_t *tq, task_t *task) {
  SCOPED_MTX_LOCK(&tq->tq_mutex);
  STAILQ_INSERT_TAIL(&tq->tq_list, task, t_link);
  cv_signal(&tq->tq_nonempty);
}

void taskqueue_run(taskqueue_t *tq) {
  task_list_t tasklist;

  WITH_MTX_LOCK (&tq->tq_mutex) {
    while (STAILQ_EMPTY(&tq->tq_list))
      cv_wait(&tq->tq_nonempty, &tq->tq_mutex);

    /* Copy the list head into a local var, and empty the original head. */
    tasklist = tq->tq_list;
    STAILQ_INIT(&tq->tq_list);
  }

  while (!STAILQ_EMPTY(&tasklist)) {
    task_t *task = STAILQ_FIRST(&tasklist);
    task->t_func(task->t_arg);
    STAILQ_REMOVE_HEAD(&tasklist, t_link);
  }
}
