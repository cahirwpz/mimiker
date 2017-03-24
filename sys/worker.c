#include <sched.h>
#include <thread.h>
#include <worker.h>

taskqueue_t *workqueue;

static void worker_invoke(void *p) {
  while (true) {
    taskqueue_run(workqueue);
    sched_yield();
  }
}

void worker_init() {
  workqueue = taskqueue_create();
  thread_t *t = thread_create("worker thread", worker_invoke, 0);
  sched_add(t);
}
