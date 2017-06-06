#include <stdc.h>
#include <ktest.h>
#include <malloc.h>
#include <taskqueue.h>

static unsigned int counter;

static void func(void *arg) {
  int n = *(int *)arg;
  /* Access to counter is intentionally not synchronized. */
  counter += n;
}

static int test_taskqueue(void) {
  taskqueue_t tq;
  task_t task0;
  task_t task1;
  task_t task2;

  counter = 0;
  int N[] = {1, 2, 3};

  taskqueue_init(&tq);
  task_init(&task0, func, N + 0);
  task_init(&task1, func, N + 1);
  task_init(&task2, func, N + 2);

  taskqueue_add(&tq, &task0);
  taskqueue_add(&tq, &task1);
  taskqueue_add(&tq, &task2);

  taskqueue_run(&tq);

  assert(counter == 1 + 2 + 3);

  taskqueue_destroy(&tq);

  return KTEST_SUCCESS;
}

KTEST_ADD(taskqueue, test_taskqueue, 0);
