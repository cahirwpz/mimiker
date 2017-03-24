#include <sched.h>
#include <thread.h>
#include <worker.h>

void worker_init() {
  thread_t *t = thread_create("worker thread", worker_invoke, 0);

  workqueue = taskqueue_create();
  sched_add(t);
}

void worker_invoke(void *p) {
  while(true) {
    taskqueue_run(workqueue);
    sched_yield();
  }
}
