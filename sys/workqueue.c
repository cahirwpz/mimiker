#include <sched.h>
#include <thread.h>
#include <workqueue.h>

taskqueue_t *workqueue;

void workqueue_init() {
  workqueue = taskqueue_create();
}
