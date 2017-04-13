#include <malloc.h>
#include <queue.h>
#include <taskqueue.h>
#include <thread.h>
#include <sched.h>

taskqueue_t workqueue;

void workqueue_init() {
  taskqueue_init(&workqueue);
}

static void taskqueue_worker_thread(void *p) {
  taskqueue_t *tq = (taskqueue_t *)p;

  mtx_lock(&tq->tq_mutex);
  while (tq->tq_alive) {
    while (STAILQ_EMPTY(&tq->tq_list))
      cv_wait(&tq->tq_nonempty, &tq->tq_mutex);
    taskqueue_run(tq);
  }
}

void taskqueue_init(taskqueue_t *tq) {
  STAILQ_INIT(&tq->tq_list);
  mtx_init(&tq->tq_mutex, MTX_RECURSE);
  cv_init(&tq->tq_nonempty, "taskqueue nonempty");
  tq->tq_worker =
    thread_create("taskqueue worker thread", taskqueue_worker_thread, tq);
  tq->tq_alive = 1;
  sched_add(tq->tq_worker);
}

void taskqueue_destroy(taskqueue_t *tq) {
  mtx_lock(&tq->tq_mutex);
  tq->tq_alive = 0;
  mtx_unlock(&tq->tq_mutex);
  thread_join(tq->tq_worker);
}

void taskqueue_add(taskqueue_t *tq, task_t *task) {
  mtx_lock(&tq->tq_mutex);
  STAILQ_INSERT_TAIL(&tq->tq_list, task, tq_link);
  cv_signal(&tq->tq_nonempty);
  mtx_unlock(&tq->tq_mutex);
}

void taskqueue_run(taskqueue_t *tq) {
  task_t *task;
  /* Copy the list head into a local var, and empty the original head. */
  mtx_lock(&tq->tq_mutex);
  tq_list_t tq_list_copy = tq->tq_list;
  STAILQ_INIT(&tq->tq_list);
  mtx_unlock(&tq->tq_mutex);

  while (!STAILQ_EMPTY(&tq_list_copy)) {
    task = STAILQ_FIRST(&tq_list_copy);
    task->func(task->arg);
    STAILQ_REMOVE_HEAD(&tq_list_copy, tq_link);
  }
}

void task_init(task_t *task, void (*func)(void *), void *arg) {
  task->func = func;
  task->arg = arg;
}
