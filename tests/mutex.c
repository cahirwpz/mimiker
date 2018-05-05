#include <stdc.h>
#include <sched.h>
#include <mutex.h>
#include <thread.h>
#include <ktest.h>

static mtx_t counter_mtx = MTX_INITIALIZER(MTX_DEF);
static volatile int32_t counter_value;

#define COUNTER_N 1000
#define COUNTER_T 5

static thread_t *counter_td[COUNTER_T];

static void counter_routine(void *arg) {
  for (size_t i = 0; i < COUNTER_N; i++) {
    mtx_lock(&counter_mtx);
    int v = counter_value;
    thread_yield();
    counter_value = v + 1;
    mtx_unlock(&counter_mtx);
  }
}

static int mtx_test_counter(void) {
  counter_value = 0;

  for (int i = 0; i < COUNTER_T; i++) {
    char name[20];
    snprintf(name, sizeof(name), "td%d", i);
    counter_td[i] = thread_create(name, counter_routine, NULL);
  }

  for (int i = 0; i < COUNTER_T; i++)
    sched_add(counter_td[i]);
  for (int i = 0; i < COUNTER_T; i++)
    thread_join(counter_td[i]);

  assert(counter_value == COUNTER_N * COUNTER_T);

  return KTEST_SUCCESS;
}

KTEST_ADD(mutex, mtx_test_counter, 0);
