#include <malloc.h>
#include <queue.h>
#include <taskqueue.h>
#include <thread.h>
#include <sched.h>

static MALLOC_DEFINE(tq_pool, "taskqueues pool");

void taskqueue_init() {
  kmalloc_init(tq_pool);
  kmalloc_add_arena(tq_pool, pm_alloc(1)->vaddr, PAGESIZE);
}

static void taskqueue_worker_thread(void *p) {
  taskqueue_t *tq = (taskqueue_t *)p;

  while (true) {
    mtx_lock(&tq->tq_mutex);
    while (STAILQ_EMPTY(&tq->tq_list))
      cv_wait(&tq->tq_nonempty, &tq->tq_mutex);
    taskqueue_run(tq);
    mtx_unlock(&tq->tq_mutex);

    sched_yield();
  }
}

taskqueue_t *taskqueue_create() {
  taskqueue_t *tq = kmalloc(tq_pool, sizeof(taskqueue_t), M_WAITOK);
  STAILQ_INIT(&tq->tq_list);
  mtx_init(&tq->tq_mutex, MTX_RECURSE);
  cv_init(&tq->tq_nonempty, "taskqueue nonempty");
  tq->tq_worker =
    thread_create("taskqueue worker thread", taskqueue_worker_thread, tq);
  sched_add(tq->tq_worker);
  return tq;
}

void taskqueue_add(taskqueue_t *tq, task_t *task) {
  mtx_lock(&tq->tq_mutex);
  STAILQ_INSERT_TAIL(&tq->tq_list, task, tq_link);
  cv_signal(&tq->tq_nonempty);
  mtx_unlock(&tq->tq_mutex);
}

void taskqueue_run(taskqueue_t *tq) {
  task_t *task;
  mtx_lock(&tq->tq_mutex);
  while (!STAILQ_EMPTY(&tq->tq_list)) {
    task = STAILQ_FIRST(&tq->tq_list);
    mtx_unlock(&tq->tq_mutex);

    task->func(task->arg);

    mtx_lock(&tq->tq_mutex);
    /* Note: Even though we unlocked the mutex for a while, task is still at the
       head of the queue, since we only append to the tail, and this is the only
       function which removes elements from the queue. */
    STAILQ_REMOVE_HEAD(&tq->tq_list, tq_link);
    kfree(tq_pool, task);
  }
  mtx_unlock(&tq->tq_mutex);
}

task_t *task_create(void (*func)(void *), void *arg) {
  task_t *task = kmalloc(tq_pool, sizeof(task_t), M_WAITOK);
  task->func = func;
  task->arg = arg;
  return task;
}
