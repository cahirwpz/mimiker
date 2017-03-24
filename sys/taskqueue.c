#include <malloc.h>
#include <queue.h>
#include <taskqueue.h>

static MALLOC_DEFINE(tq_pool, "taskqueue pool");

void taskqueue_init() {
  kmalloc_init(tq_pool);
  kmalloc_add_arena(tq_pool, pm_alloc(1)->vaddr, PAGESIZE);
}

taskqueue_t *taskqueue_create() {
  taskqueue_t *tq = kmalloc(tq_pool, sizeof(taskqueue_t), M_WAITOK);
  STAILQ_INIT(tq);
  return tq;
}

void taskqueue_add(taskqueue_t *tq, task_t *task) {
  STAILQ_INSERT_TAIL(tq, task, tq_link);
}

void taskqueue_run(taskqueue_t *tq) {
  task_t *task;

  while (!STAILQ_EMPTY(tq)) {
    task = STAILQ_FIRST(tq);
    task->func(task->arg);
    STAILQ_REMOVE_HEAD(tq, tq_link);
  }
}

task_t *task_create(void (*func)(void *), void *arg) {
  task_t *task = kmalloc(tq_pool, sizeof(task_t), M_WAITOK);
  task->func = func;
  task->arg = arg;
  return task;
}
